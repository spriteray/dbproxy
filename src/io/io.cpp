
#include <cassert>

#include "io.h"

sid_t IIOSession::id() const
{
	return m_Sid;
}

void * IIOSession::localdata() const
{
    return m_LocalData;
}

void IIOSession::setTimeout( int32_t seconds )
{
	assert( m_Sid != 0 && m_Layer != NULL );
	iolayer_set_timeout( m_Layer, m_Sid, seconds );
}

void IIOSession::setKeepalive( int32_t seconds )
{
	assert( m_Sid != 0 && m_Layer != NULL );
	iolayer_set_keepalive( m_Layer, m_Sid, seconds );
}

int32_t IIOSession::send( const std::string & buffer )
{
	return send( buffer.c_str(), static_cast<uint32_t>(buffer.length()) );
}

int32_t IIOSession::send( const char * buffer, uint32_t nbytes, bool isfree )
{
	return iolayer_send( m_Layer, m_Sid, buffer, nbytes, static_cast<int32_t>(isfree) );
}

int32_t IIOSession::shutdown()
{
	return iolayer_shutdown( m_Layer, m_Sid );
}

void IIOSession::init( sid_t id, void * local, iolayer_t layer )
{
	m_Sid       = id;
	m_LocalData = local;
	m_Layer     = layer;
}

int32_t IIOSession::onStartSession( void * context )
{
	return static_cast<IIOSession *>(context)->onStart();
}

int32_t IIOSession::onProcessSession( void * context, const char * buffer, uint32_t nbytes )
{
	return static_cast<IIOSession *>(context)->onProcess( buffer, nbytes );
}

char * IIOSession::onTransformSession( void * context, const char * buffer, uint32_t * nbytes )
{
	uint32_t & _nbytes = *nbytes;
	return static_cast<IIOSession *>(context)->onTransform( buffer, _nbytes );
}

int32_t IIOSession::onTimeoutSession( void * context )
{
	return static_cast<IIOSession *>(context)->onTimeout();
}

int32_t IIOSession::onKeepaliveSession( void * context )
{
	return static_cast<IIOSession *>(context)->onKeepalive();
}

int32_t IIOSession::onErrorSession( void * context, int32_t result )
{
	return static_cast<IIOSession *>(context)->onError( result );
}

void IIOSession::onShutdownSession( void * context, int32_t way )
{
	IIOSession * session = static_cast<IIOSession *>( context );
	session->onShutdown( way );
	delete session;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

IIOService::IIOService( uint8_t nthreads, uint32_t nclients )
    : m_IOLayer(NULL),
      m_ThreadsCount( nthreads ),
      m_SessionsCount( nclients )
{
	m_IOLayer = iolayer_create( m_ThreadsCount, m_SessionsCount );

	if ( m_IOLayer != NULL )
	{
	    iolayer_set_localdata( m_IOLayer, getThreadLocalData, this );
        iolayer_set_transform( m_IOLayer, onTransformService, this );
	}
}

IIOService::~IIOService()
{
    if ( m_IOLayer != NULL )
    {
        iolayer_destroy( m_IOLayer );
        m_IOLayer = NULL;
    }
}

bool IIOService::listen( const char * host, uint16_t port )
{
	return ( iolayer_listen( m_IOLayer, host, port, onAcceptSession, this ) == 0 );
}

bool IIOService::connect( const char * host, uint16_t port, int32_t seconds )
{
	return ( iolayer_connect( m_IOLayer, host, port, seconds, onConnectSession, this ) == 0 );
}

void IIOService::stop()
{
    if ( m_IOLayer != NULL )
    {
        iolayer_stop( m_IOLayer );
    }
}

int32_t IIOService::send( sid_t id, const std::string & buffer )
{
	return send( id, buffer.c_str(), static_cast<uint32_t>(buffer.length()) );
}

int32_t IIOService::send( sid_t id, const char * buffer, uint32_t nbytes, bool isfree )
{
	return iolayer_send( m_IOLayer, id, buffer, nbytes, isfree );
}

int32_t IIOService::broadcast( const std::vector<sid_t> & ids, const std::string & buffer )
{
	return broadcast( ids, buffer.c_str(), static_cast<uint32_t>(buffer.length()) );
}

int32_t IIOService::broadcast( const std::vector<sid_t> & ids, const char * buffer, uint32_t nbytes )
{
	std::vector<sid_t>::const_iterator start = ids.begin();

	uint32_t count = (uint32_t)ids.size();
	sid_t * idlist = const_cast<sid_t *>( &(*start) );

	return iolayer_broadcast( m_IOLayer, idlist, count, buffer, nbytes );
}

int32_t IIOService::shutdown( sid_t id )
{
	return iolayer_shutdown( m_IOLayer, id );
}

int32_t IIOService::shutdown( const std::vector<sid_t> & ids )
{
	std::vector<sid_t>::const_iterator start = ids.begin();

	uint32_t count = (uint32_t)ids.size();
	sid_t * idlist = const_cast<sid_t *>( &(*start) );

	return iolayer_shutdowns( m_IOLayer, idlist, count );
}

void IIOService::attach( sid_t id, IIOSession * session, void * local )
{
	session->init( id, local, m_IOLayer );

	ioservice_t ioservice;
	ioservice.start		= IIOSession::onStartSession;
	ioservice.process	= IIOSession::onProcessSession;
	ioservice.transform = IIOSession::onTransformSession;
	ioservice.timeout	= IIOSession::onTimeoutSession;
	ioservice.keepalive	= IIOSession::onKeepaliveSession;
	ioservice.error		= IIOSession::onErrorSession;
	ioservice.shutdown	= IIOSession::onShutdownSession;
	iolayer_set_service( m_IOLayer, id, &ioservice, session );
}

void * IIOService::getThreadLocalData( void * context, uint8_t index )
{
    return static_cast<IIOService *>(context)->getLocalData( index );
}

char * IIOService::onTransformService( void * context, const char * buffer, uint32_t * nbytes )
{
    uint32_t & _nbytes = *nbytes;
    IIOService * service = static_cast<IIOService*>( context );

    return service->onTransform( buffer, _nbytes );
}

int32_t IIOService::onAcceptSession( void * context, void * local, sid_t id, const char * host, uint16_t port )
{
	IIOSession * session = NULL;
	IIOService * service = static_cast<IIOService*>( context );

	session = service->onAccept( id, host, port );
	if ( session == NULL )
	{
		return -1;
	}
	service->attach( id, session, local );

	return 0;
}

int32_t IIOService::onConnectSession( void * context, void * local, int32_t result, const char * host, uint16_t port, sid_t id )
{
	IIOSession * session = NULL;
	IIOService * service = static_cast<IIOService *>( context );

	if ( result != 0 )
	{
        return service->onConnectError( result, host, port ) ? 0 : -2;
	}

	session = service->onConnect( id, host, port );
	if ( session == NULL )
	{
		return -1;
	}
	service->attach( id, session, local );

	return 0;
}
