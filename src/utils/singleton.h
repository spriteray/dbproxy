
#ifndef __SRC_UTILS_SINGLETON_H__
#define __SRC_UTILS_SINGLETON_H__

template<typename T>
class Singleton
{
public:
    static T & getInstance()
    {
        if ( m_Instance == 0 )
        {
            m_Instance = new T();
        }

        return *m_Instance;
    }

    static void delInstance()
    {
        if( m_Instance != 0 )
        {
            delete m_Instance;
            m_Instance = 0;
        }
    }

protected:
    Singleton() {}
    ~Singleton() {}

private:
    Singleton( const Singleton & );
    Singleton & operator = ( const Singleton & );

private:
    static T * m_Instance;
};

template<typename T>
T * Singleton<T>::m_Instance = 0;

#endif
