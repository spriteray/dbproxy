#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re
import sys
import MySQLdb

HOST='${HOST}'
USERNAME='${USERNAME}'
PASSWORD='${PASSWORD}'
DATABASE='${DATABASE}'
PORT=3306

class MySQLTable() :
    def __init__( self, table ) :
        self._keys = []
        self._fields = []
        self._aifield = ''
        self._table = table

    def get_descs( self, db ) :
        index = 0
        db.execute( 'desc %s' % self._table )
        results = db.fetchall()
        for result in results :
            field = result[0]
            cpptype = to_cpptype( result[1] )
            self._fields.append( (field, cpptype, index) )
            index = index+1
            if result[3] == 'PRI' :
                self._keys.append( field )
            if result[5] == 'auto_increment' :
                self._aifield = field

    def get_field( self, key ) :
        for field in self._fields :
            if field[0] == key:
                return field
        return ( key, 'std::string', 0 )

    def get_keys_values( self ) :
        keys = ''
        values = ''
        for key in self._keys:
            field = self.get_field( key )
            values = values + 'm_%s, ' % field[0]
            keys = keys + '%s=\'%s\' AND ' % ( field[0], get_placeholder(field[1]) )
        keys = keys[:-5]
        values = values[:-2]
        return keys, values

    def generate( self ) :
        self.generate_hpp()
        self.generate_cpp()

    def generate_hpp( self ) :
        tofile = open( '%s.h' % self._table, 'w' )
        tofile.write( '\n' )
        tofile.write( '#ifndef __SQLBIND_GENERATE_%s_H__\n' % self._table.upper() )
        tofile.write( '#define __SQLBIND_GENERATE_%s_H__\n' % self._table.upper() )
        tofile.write( '\n' )
        tofile.write( '#include \"sqlbind.h\"\n' )
        tofile.write( '\n' )
        tofile.write( 'class %s : public SQLBind::ISQLBind\n' % self._table )
        tofile.write( '{\n' )
        tofile.write( 'public :\n' )
        tofile.write( '    %s();\n' % self._table )
        tofile.write( '    virtual ~%s();\n' % self._table )
        tofile.write( '\n' )
        tofile.write( '    virtual std::string insert();\n');
        tofile.write( '    virtual std::string query();\n');
        tofile.write( '    virtual std::string update();\n');
        tofile.write( '    virtual std::string remove();\n');
        tofile.write( '    virtual bool parse( const SQLBind::Result & result );\n');
        tofile.write( '    virtual void autoincrease( const std::string & result );\n');
        tofile.write( '\n' )
        tofile.write( 'public :\n' )
        tofile.write( '    // clean dirty flags\n' );
        tofile.write( '    void clean() { std::memset( &m_dirty, 0, sizeof(m_dirty) ); }\n' )
        tofile.write( '\n' )
        for field in self._fields :
            self.generate_func( field, tofile )
        tofile.write( 'private :\n' )
        tofile.write( '    enum { NFIELDS = %d };\n' % len(self._fields) )
        tofile.write( '    int8_t m_dirty[ NFIELDS ];\n' )
        tofile.write( '\n' )
        tofile.write( 'private :\n' )
        for field in self._fields :
            tofile.write( '    %s m_%s;\n' % ( field[1], field[0] ) )
        tofile.write( '};\n' )
        tofile.write( '\n' )
        tofile.write( '#endif' )
        tofile.close()

    def generate_cpp( self ) :
        tofile = open( '%s.cc' % self._table, 'w' )
        tofile.write( '\n' )
        tofile.write( '#include <cstdio>\n' )
        tofile.write( '#include <cassert>\n' )
        tofile.write( '#include <cstdlib>\n' )
        tofile.write( '#include \"%s.h\"\n' % self._table )
        tofile.write( '\n' )
        tofile.write( '%s::%s()\n' % ( self._table, self._table)  )
        tofile.write( '{\n' );
        for field in self._fields :
            if field[1] != 'std::string' :
                tofile.write( '    m_%s = 0;\n' % field[0] )
        tofile.write( '    this->clean();\n' );
        tofile.write( '}\n' );
        tofile.write( '\n' )
        tofile.write( '%s::~%s()\n' % ( self._table, self._table)  )
        tofile.write( '{}\n' )
        tofile.write( '\n' )
        self.generate_insert( tofile )
        self.generate_query( tofile )
        self.generate_update( tofile )
        self.generate_remove( tofile )
        self.generate_parse( tofile )
        self.generate_autoincrease( tofile )
        tofile.close()

    def generate_func( self, field, tofile ) :
        tofile.write( '    // Field: %s, Index: %d\n' % (field[0], field[2]) )
        if field[1] == 'std::string' :
            tofile.write( '    const std::string & %s() const { return m_%s; }\n' % ( field[0], field[0] ) )
            tofile.write( '    void set_%s( const std::string & value ) { m_%s = value; m_dirty[%d] = 1; }\n' % ( field[0], field[0], field[2] ) )
        else :
            tofile.write( '    %s %s() const { return m_%s; }\n' % ( field[1], field[0], field[0] ) )
            tofile.write( '    void set_%s( %s value ) { m_%s = value; m_dirty[%d] = 1; }\n' % ( field[0], field[1], field[0], field[2] ) )
        tofile.write( '\n' )

    def generate_insert( self, tofile ) :
        tofile.write( 'std::string %s::insert()\n' % self._table )
        tofile.write( '{\n' )
        tofile.write( '    char sqlcmd[ 4096 ];\n' )
        tofile.write( '    char fields[ 2048 ] = { 0 };\n' )
        tofile.write( '    char values[ 2048 ] = { 0 };\n' )
        tofile.write( '\n' )
        tofile.write( '    for ( uint32_t i = 0; i < NFIELDS; ++i )\n' )
        tofile.write( '    {\n' )
        tofile.write( '        char change[ 512 ] = { 0 };\n' )
        tofile.write( '\n' )
        tofile.write( '        if ( m_dirty[i] == 0 )\n' )
        tofile.write( '        {\n' )
        tofile.write( '            continue;\n' )
        tofile.write( '        }\n' )
        tofile.write( '\n' )
        tofile.write( '        if ( std::strlen( fields ) > 0 )\n' )
        tofile.write( '        {\n' )
        tofile.write( '            std::strcat( fields, \", \" );\n' )
        tofile.write( '        }\n' )
        tofile.write( '        if ( std::strlen( values ) > 0 )\n' )
        tofile.write( '        {\n' )
        tofile.write( '            std::strcat( values, \", \" );\n' )
        tofile.write( '        }\n' )
        tofile.write( '\n' )
        tofile.write( '        switch ( i )\n' )
        tofile.write( '        {\n' )
        for field in self._fields :
            tofile.write( '            case %s :\n' % field[2] );
            tofile.write( '                std::strcat( fields, \"%s\" );\n' % field[0] );
            if field[1] != 'std::string' :
                tofile.write( '                std::snprintf( change, 511, \"\'%s\'\", m_%s );\n' % (get_placeholder(field[1]), field[0]) );
            else :
                tofile.write( '                std::snprintf( change, 511, \"\'%s\'\", m_%s.c_str() );\n' % (get_placeholder(field[1]), field[0]) );
            tofile.write( '                break;\n' )
        tofile.write( '        }\n' )
        tofile.write( '\n' )
        tofile.write( '        std::strcat( values, change );\n' )
        tofile.write( '    }\n' )
        tofile.write( '    this->clean();\n' )
        tofile.write( '\n' )
        tofile.write( '    std::snprintf( sqlcmd, 4095,\n' )
        tofile.write( '            \"INSERT INTO %s (%s) VALUES (%s)\",\n' % ( self._table, '%s', '%s' ) )
        tofile.write( '            fields, values );\n' )
        tofile.write( '\n' )
        tofile.write( '    return sqlcmd;\n' )
        tofile.write( '}\n' )
        tofile.write( '\n' )

    def generate_query( self, tofile ) :
        keys, values = self.get_keys_values()
        tofile.write( 'std::string %s::query()\n' % self._table )
        tofile.write( '{\n' )
        tofile.write( '    char sqlcmd[ 4096 ];\n' )
        tofile.write( '    std::snprintf( sqlcmd, 4095,\n' )
        tofile.write( '            \"SELECT * FROM %s WHERE %s\",\n' % ( self._table, keys ) )
        tofile.write( '            %s );\n' % values )
        tofile.write( '    return sqlcmd;\n' )
        tofile.write( '}\n' )
        tofile.write( '\n' )

    def generate_update( self, tofile ) :
        keys, values = self.get_keys_values()
        tofile.write( 'std::string %s::update()\n' % self._table )
        tofile.write( '{\n' )
        tofile.write( '    char sqlcmd[ 4096 ];\n' )
        tofile.write( '    char dirty[ 2048 ] = { 0 };\n' )
        tofile.write( '\n' )
        tofile.write( '    for ( uint32_t i = 0; i < NFIELDS; ++i )\n' )
        tofile.write( '    {\n' )
        tofile.write( '        char change[ 512 ] = {0};\n' )
        tofile.write( '\n' )
        tofile.write( '        if ( m_dirty[i] == 0 )\n' )
        tofile.write( '        {\n' )
        tofile.write( '            continue;\n' )
        tofile.write( '        }\n' )
        tofile.write( '\n' )
        tofile.write( '        if ( std::strlen(dirty) > 0 )\n' )
        tofile.write( '        {\n' )
        tofile.write( '            std::strcat( dirty, \", \" );\n' )
        tofile.write( '        }\n' )
        tofile.write( '\n' )
        tofile.write( '        switch ( i )\n' )
        tofile.write( '        {\n' )
        for field in self._fields :
            tofile.write( '            case %s :\n' % field[2] );
            if field[1] != 'std::string' :
                tofile.write( '                std::snprintf( change, 511, \"%s=\'%s\'\", m_%s );\n' % ( field[0], get_placeholder(field[1]), field[0] ) )
            else :
                tofile.write( '                std::snprintf( change, 511, \"%s=\'%s\'\", m_%s.c_str() );\n' % ( field[0], get_placeholder(field[1]), field[0] ) )
            tofile.write( '                break;\n' )
        tofile.write( '        }\n' )
        tofile.write( '\n' )
        tofile.write( '        std::strcat( dirty, change );\n' )
        tofile.write( '    }\n' )
        tofile.write( '    this->clean();\n' )
        tofile.write( '\n' )
        tofile.write( '    std::snprintf( sqlcmd, 4095,\n' )
        tofile.write( '            \"UPDATE %s SET %s WHERE %s\",\n' % ( self._table, '%s', keys ) )
        tofile.write( '            dirty, %s );\n' % values )
        tofile.write( '\n' )
        tofile.write( '    return sqlcmd;\n' )
        tofile.write( '}\n' )
        tofile.write( '\n' )

    def generate_remove( self, tofile ) :
        keys, values = self.get_keys_values()
        tofile.write( 'std::string %s::remove()\n' % self._table )
        tofile.write( '{\n' )
        tofile.write( '    char sqlcmd[ 4096 ];\n' )
        tofile.write( '    std::snprintf( sqlcmd, 4095,\n' )
        tofile.write( '            \"DELETE FROM %s WHERE %s\",\n' % ( self._table, keys ) )
        tofile.write( '            %s );\n' % values )
        tofile.write( '    return sqlcmd;\n' )
        tofile.write( '}\n' )
        tofile.write( '\n' )

    def generate_parse( self, tofile ) :
        tofile.write( 'bool %s::parse( const SQLBind::Result & result )\n' % self._table )
        tofile.write( '{\n' )
        tofile.write( '    assert( result.size() == NFIELDS );\n' )
        tofile.write( '\n' )
        tofile.write( '    for ( uint32_t i = 0; i < NFIELDS; ++i )\n' )
        tofile.write( '    {\n' )
        tofile.write( '        switch ( i )\n' )
        tofile.write( '        {\n' )
        for field in self._fields :
            tofile.write( '            case %s :\n' % field[2] );
            if field[1] == 'int64_t' :
                tofile.write( '                m_%s = (%s)std::atoll( result[i].c_str() );\n' % (field[0], field[1]) );
            elif field[1] == 'uint64_t' :
                tofile.write( '                m_%s = (%s)std::atoll( result[i].c_str() );\n' % (field[0], field[1]) );
            elif field[1] == 'std::string' :
                tofile.write( '                m_%s = result[i];\n' % field[0] );
            else :
                tofile.write( '                m_%s = (%s)std::atoi( result[i].c_str() );\n' % (field[0], field[1]) );
            tofile.write( '                break;\n' )
        tofile.write( '        }\n' )
        tofile.write( '    }\n' )
        tofile.write( '\n' )
        tofile.write( '    return true;\n' )
        tofile.write( '}\n' )
        tofile.write( '\n' )

    def generate_autoincrease( self, tofile ) :
        tofile.write( 'void %s::autoincrease( const std::string & result )\n' % self._table )
        tofile.write( '{\n' )
        if self._aifield != '' :
            field = self.get_field( self._aifield )
            if field[1] == 'int64_t' :
                tofile.write( '    m_%s = (%s)std::atoll( result.c_str() );\n' % (field[0], field[1]) );
            elif field[1] == 'uint64_t' :
                tofile.write( '    m_%s = (%s)std::atoll( result.c_str() );\n' % (field[0], field[1]) );
            elif field[1] == 'std::string' :
                tofile.write( '    m_%s = result;\n' % field[0] );
            else :
                tofile.write( '    m_%s = (%s)std::atoi( result.c_str() );\n' % (field[0], field[1]) );
        tofile.write( '}\n' )
        tofile.write( '\n' )

# MYSQL to CPP
def to_cpptype( dbtype ) :
    cpptype = ''
    mytype = re.split( r'\(.+\)', dbtype )
    if mytype[0] == 'tinyint' :
        if mytype[1] == ' unsigned' : cpptype = 'uint8_t'
        else : cpptype = 'int8_t'
    elif mytype[0] == 'smallint' :
        if mytype[1] == ' unsigned' : cpptype = 'uint16_t'
        else : cpptype = 'int16_t'
    elif mytype[0] == 'int' :
        if mytype[1] == ' unsigned' : cpptype = 'uint32_t'
        else : cpptype = 'int32_t'
    elif mytype[0] == 'bigint' :
        if mytype[1] == ' unsigned' : cpptype = 'uint64_t'
        else : cpptype = 'int64_t'
    elif mytype[0] == 'timestamp' :
        cpptype = 'int64_t'
    else :
        cpptype = 'std::string'
    return cpptype

# place holder
def get_placeholder( cpptype ) :
    holder = ''
    if cpptype == 'int8_t' : holder = '%d'
    elif cpptype == 'uint8_t' : holder = '%u'
    elif cpptype == 'int16_t' : holder = '%d'
    elif cpptype == 'uint16_t' : holder = '%u'
    elif cpptype == 'int32_t' : holder = '%d'
    elif cpptype == 'uint32_t' : holder = '%u'
    elif cpptype == 'int64_t' : holder = '%ld'
    elif cpptype == 'uint64_t' : holder = '%lu'
    else : holder = '%s'
    return holder

def scan_database( database ) :
    conn = MySQLdb.connect( host=HOST, user=USERNAME, passwd=PASSWORD, db=database, port=PORT )
    handler = conn.cursor()
    handler.execute( 'show tables' )
    results = handler.fetchall()
    for result in results :
        print 'Bind Table:%s' % result[0]
        table = MySQLTable( result[0] )
        table.get_descs( handler )
        table.generate()
    handler.close()
    conn.close()

if __name__ == "__main__" :
    reload(sys)
    sys.setdefaultencoding('utf-8')
    scan_database( DATABASE )
