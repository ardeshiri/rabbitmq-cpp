#include "CPPAMQP.h"
#include <exception>
#include <iostream>
#include <vector>
#include <cstring>

arax::CPPAMQP::CPPAMQP(std::string h_name, int prt, int socket_create_timeout=0):hostname{h_name},port{prt},
    timeout{socket_create_timeout},socket{nullptr},conn{},tval{},tv{nullptr},
    q_names{},stts{RMQ_stts::DC}
{
    if (timeout > 0)
    {
        tv = &tval;
        tv->tv_usec = timeout;
    }
    else
    {
        tv = NULL;
    }
}

arax::CPPAMQP::CPPAMQP(CPPAMQP&& rvi):hostname{std::move(rvi.hostname)}, port{rvi.port}, timeout{rvi.timeout}, socket{rvi.socket},conn{std::move(rvi.conn)},
                                      tval{std::move(rvi.tval)}, tv{rvi.tv}, q_names{std::move(rvi.q_names)}, stts{rvi.stts}
{
  rvi.socket = nullptr;
  rvi.tv = nullptr;
  rvi.stts = RMQ_stts::DC;
}

arax::CPPAMQP& arax::CPPAMQP::operator=(CPPAMQP&& rvi)
{
    this->~CPPAMQP();
    hostname = std::move(rvi.hostname);
    port = rvi.port;
    rvi.port = 0;
    timeout = rvi.timeout;
    rvi.timeout = 0;
    socket = rvi.socket;
    rvi.socket = nullptr;
    conn = std::move(rvi.conn);
    tval = std::move(rvi.tval);
    tv = rvi.tv;
    rvi.tv = nullptr;
    q_names = std::move(rvi.q_names);
    stts = rvi.stts;
    rvi.stts = RMQ_stts::DC;
    return *this;
}


arax::CPPAMQP::~CPPAMQP()
{
    if(stts==RMQ_stts::normal_C)
        close_normal();
    if(stts==RMQ_stts::SSL_C)
        close_ssl();
}


void arax::CPPAMQP::connect_normal(std::string vhost, std::string user_name, std::string password,  int channel_max, int frame_max, int heartbeat, amqp_sasl_method_enum sasl_method)
{
	conn = amqp_new_connection();
    socket = amqp_tcp_socket_new(conn);
    if (!socket)
    {
        throw RMQ_exception("creating TCP socket");
    }
    int status = amqp_socket_open(socket, hostname.c_str(), port);
    if (status)
    {
        throw RMQ_exception("opening TCP socket");
    }
    die_on_amqp_error(amqp_login(conn, vhost.c_str(), channel_max, frame_max, heartbeat, sasl_method, user_name.c_str(), password.c_str()),"Logging in");
    amqp_channel_open(conn, 1);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
    stts=RMQ_stts::normal_C;
}


void arax::CPPAMQP::close_normal()
{
    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                      "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                      "Closing connection");
    die_on_error(amqp_destroy_connection(conn),
                 "Ending connection");
    stts=RMQ_stts::DC;
}


void arax::CPPAMQP::listen_to_queue(amqp_channel_t channel,const std::string& queue_name,const amqp_bytes_t& consumer_tag, amqp_boolean_t no_local,
                                    amqp_boolean_t no_ack, amqp_boolean_t exclusive,const amqp_table_t& arguments )
{
    if(stts == RMQ_stts::DC)
    {
        throw RMQ_exception{"not connected"};
    }
    amqp_basic_consume(conn, channel, amqp_cstring_bytes(queue_name.c_str()), consumer_tag,
                       no_local, no_ack, exclusive, arguments);
    q_names.insert(queue_name);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");
}


std::vector<arax::amqp_envlp_t> arax::CPPAMQP::receive(const int min_envs, struct timeval *tvp)
{
    if(stts == RMQ_stts::DC)
    {
        throw RMQ_exception{"not connected"};
    }
    std::vector<amqp_envlp_t> env_vec{};
    int counter = 0;
    for(;;)
    {
        amqp_rpc_reply_t res;
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(conn);
        res = amqp_consume_message(conn, &envelope, (tvp != nullptr)? tvp : NULL, 0);

        if (AMQP_RESPONSE_NORMAL != res.reply_type)
        {
            break;
        }
        amqp_envlp_t ev{};
        env_copy( ev, envelope);
        env_vec.push_back(std::move(ev));
        if(amqp_basic_ack(conn, envelope.channel, envelope.delivery_tag, 0) != 0 )
        {
            throw RMQ_exception{"basic.ack"};
        }
        amqp_destroy_envelope(&envelope);
        ++counter;
        if(min_envs!=0 && counter == min_envs)
            return env_vec;
    }
    return env_vec;
}


std::vector<std::tuple<std::string,std::string,std::string>>  arax::CPPAMQP::receive_string(const int min_envs, struct timeval *tvp)
{
    if(stts == RMQ_stts::DC)
    {
        throw RMQ_exception{"not connected"};
    }
    std::vector<std::tuple<std::string,std::string,std::string>>  str_vec{};
    auto env_vec = receive( min_envs, tvp);
    for(auto env: env_vec)
    {
        std::string str_msg{};
        std::copy( env.message.body.data(), env.message.body.data()+env.message.body.size(), std::back_inserter(str_msg));
        std::string str_exch{};
        std::copy( env.exchange.data(), env.exchange.data()+env.exchange.size(), std::back_inserter(str_exch));
        std::string str_rk{};
        std::copy( env.routing_key.data(), env.routing_key.data()+env.routing_key.size(), std::back_inserter(str_rk));
        str_vec.emplace_back(str_exch,str_rk,str_msg);
    }
    return str_vec;
}


void arax::CPPAMQP::receive_stream(std::ostream& os, struct timeval *tvp, const std::string routing_k, const std::string exchange)
{
    if(stts == RMQ_stts::DC)
    {
        throw RMQ_exception{"not connected"};
    }
    for(;;)
    {
        auto env_vec = receive( RECEIVE_STREAM_LOOP, tvp);
        if(env_vec.size() == 0)
        {
            return;
        }
        for(auto env: env_vec)
        {
            if(routing_k.size() == 0)
            {
                if(exchange.size() == 0)
                {
                    os.write(reinterpret_cast<char*>(env.message.body.data()),env.message.body.size());
                }
                else
                {
                    std::string str_exch{};
                    std::copy( env.exchange.data(), env.exchange.data()+env.exchange.size(), std::back_inserter(str_exch));
                    if( exchange.compare(str_exch) == 0)
                        os.write(reinterpret_cast<char*>(env.message.body.data()),env.message.body.size());
                }
            }
            else
            {
                if(exchange.size() == 0)
                {
                    std::string str_rk{};
                    std::copy( env.routing_key.data(), env.routing_key.data()+env.routing_key.size(), std::back_inserter(str_rk));
                    if(routing_k.compare(str_rk) == 0)
                        os.write(reinterpret_cast<char*>(env.message.body.data()),env.message.body.size());
                }
                else
                {
                    std::string str_rk{};
                    std::copy( env.routing_key.data(), env.routing_key.data()+env.routing_key.size(), std::back_inserter(str_rk));
                    std::string str_exch{};
                    std::copy( env.exchange.data(), env.exchange.data()+env.exchange.size(), std::back_inserter(str_exch));
                    if( (exchange.compare(str_exch) == 0)&&(str_rk.compare(routing_k) == 0) )
                        os.write(reinterpret_cast<char*>(env.message.body.data()),env.message.body.size());
                }
            }
        }
    }
}

void arax::CPPAMQP::env_copy(amqp_envlp_t& cppenv, const amqp_envelope_t& env)
{
    cppenv.channel = env.channel;
    cppenv.delivery_tag = env.delivery_tag;
    cppenv.redelivered = env.redelivered;
    std::copy( reinterpret_cast<unsigned char*>(env.consumer_tag.bytes), reinterpret_cast<unsigned char*>(env.consumer_tag.bytes)+env.consumer_tag.len, std::back_inserter(cppenv.consumer_tag));
    std::copy( reinterpret_cast<unsigned char*>(env.exchange.bytes), reinterpret_cast<unsigned char*>(env.exchange.bytes)+env.exchange.len, std::back_inserter(cppenv.exchange));
    std::copy( reinterpret_cast<unsigned char*>(env.routing_key.bytes), reinterpret_cast<unsigned char*>(env.routing_key.bytes)+env.routing_key.len, std::back_inserter(cppenv.routing_key));
    /// todo
    std::copy( reinterpret_cast<unsigned char*>(env.message.body.bytes), reinterpret_cast<unsigned char*>(env.message.body.bytes)+env.message.body.len, std::back_inserter(cppenv.message.body));
}


void arax::CPPAMQP::send_data(const std::vector< std::vector<unsigned char>>& input_v, const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
                         const amqp_boolean_t mandatory,const amqp_boolean_t immediate)
{
    if(stts == RMQ_stts::DC)
    {
        throw RMQ_exception{"not connected"};
    }
    amqp_bytes_t message_bytes{};
    for(auto msg: input_v)
    {
        message_bytes.len = msg.size();
        message_bytes.bytes = msg.data();
        die_on_error(amqp_basic_publish(conn, channel, amqp_cstring_bytes(exchange.c_str()),
                                        amqp_cstring_bytes(routing_key.c_str()), mandatory, immediate, NULL,
                                        message_bytes), "Publishing");
    }
}


void arax::CPPAMQP::send_string(const std::vector<std::string>& input_v, const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
                                const amqp_boolean_t mandatory,const amqp_boolean_t immediate)
{
    if(stts == RMQ_stts::DC)
    {
        throw RMQ_exception{"not connected"};
    }

    amqp_bytes_t message_bytes{};
    for(auto msg: input_v)
    {
        message_bytes.len = msg.size();
        message_bytes.bytes = msg.data();
        die_on_error(amqp_basic_publish(conn, channel, amqp_cstring_bytes(exchange.c_str()),
                                        amqp_cstring_bytes(routing_key.c_str()), mandatory, immediate, NULL,
                                        message_bytes), "Publishing");
    }
}


void arax::CPPAMQP::send_stream(std::istream& is,const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
                                const amqp_boolean_t mandatory,const amqp_boolean_t immediate)
{
    if(stts == RMQ_stts::DC)
    {
        throw RMQ_exception{"not connected"};
    }
    while(is && !is.eof())
    {
        std::string data_str{};
        data_str.resize(SEND_STREAM_BUFFER_SIZE);
        is.read(data_str.data(), data_str.size());
        data_str.resize(is.gcount());
        std::vector<std::string> send_data{};
        send_data.push_back(std::move(data_str));
        send_string(send_data, channel,exchange,routing_key,mandatory,immediate);
    }
}


void arax::CPPAMQP::connect_ssl(std::string vhost,std::string user_name, std::string password,const char *cacert,const char *cert, const char *key,
                                int channel_max, int frame_max, int heartbeat, amqp_sasl_method_enum sasl_method )
{
	 conn = amqp_new_connection();
    socket = amqp_ssl_socket_new(conn);
    if (!socket)
    {
        throw RMQ_exception{"creating SSL/TLS socket"};
    }
    amqp_ssl_socket_set_verify_peer(socket, 0);
    amqp_ssl_socket_set_verify_hostname(socket, 0);

    die_on_error(amqp_ssl_socket_set_cacert(socket, cacert),"setting CA certificate");
	 //amqp_ssl_socket_set_verify_peer(socket, 1);
	 //amqp_ssl_socket_set_verify_hostname(socket, 1);
	 die_on_error(amqp_ssl_socket_set_key(socket, cert, key),"setting client key");

    die_on_error(amqp_socket_open_noblock(socket, hostname.c_str(), port, tv),"opening SSL/TLS connection");
    die_on_amqp_error(amqp_login(conn, vhost.c_str(), channel_max, frame_max, heartbeat, sasl_method,user_name.c_str(), password.c_str()),"Logging in");
    stts=RMQ_stts::SSL_C;
}


void arax::CPPAMQP::close_ssl()
{
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                      "Closing connection");
    die_on_error(amqp_destroy_connection(conn),
                 "Ending connection");
    die_on_error(amqp_uninitialize_ssl_library(),
                 "Uninitializing SSL library");
    stts=RMQ_stts::DC;
}



void arax::CPPAMQP::die_on_error(int x, char const *context)
{
    if (x < 0)
    {
        std::cerr<<context<<" _ "<<amqp_error_string2(x)<<std::endl;
        throw RMQ_exception{context};
    }
}



void arax::CPPAMQP::die_on_amqp_error(amqp_rpc_reply_t x, char const *context)
{
    switch (x.reply_type)
    {
    case AMQP_RESPONSE_NORMAL:
        return;

    case AMQP_RESPONSE_NONE:
        std::cerr<<"missing RPC reply type"<<" _ "<<context<<std::endl;
        throw RMQ_exception{context};
        break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        std::cerr<<context<<" _ "<<amqp_error_string2(x.library_error)<<std::endl;
        throw RMQ_exception{context};
        break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (x.reply.id)
        {
        case AMQP_CONNECTION_CLOSE_METHOD:
        {
            amqp_connection_close_t *m =
                (amqp_connection_close_t *)x.reply.decoded;
            std::cerr<< context << " _ server connection error _ " << m->reply_code << " _ message: "<< (char *)m->reply_text.bytes<<std::endl;
            throw RMQ_exception{context};
            break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD:
        {
            amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
            std::cerr<< context <<" _ server channel error _ " << m->reply_code << " _ message: "<< (char *)m->reply_text.bytes<<std::endl;
            throw RMQ_exception{context};
            break;
        }
        default:
            std::cerr<<context<<" _ unknown server error, method id "<<x.reply.id;
            throw RMQ_exception{"unknown server error"};
            break;
        }
        break;
    }
    exit(1);
}


