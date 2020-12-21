#ifndef CPPAMQP_H
#define CPPAMQP_H
#include <amqp.h>
#include <amqp_ssl_socket.h>
#include <amqp_tcp_socket.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <iostream>
namespace arax{

struct amqp_tbl_entry_t_ {
  std::vector<unsigned char> key;
  amqp_field_value_t value;
};
using amqp_tbl_entry_t = amqp_tbl_entry_t_;

struct amqp_tbl_t_ {
  std::vector<amqp_tbl_entry_t> key;
};
using amqp_tbl_t = amqp_tbl_t_;

struct amqp_basic_prop
{
    amqp_flags_t flags;
    std::vector<unsigned char> content_type;
    std::vector<unsigned char> content_encoding;
    uint8_t delivery_mode;
    uint8_t priority;
    std::vector<unsigned char> correlation_id;
    std::vector<unsigned char> reply_to;
    std::vector<unsigned char> expiration;
    std::vector<unsigned char> message_id;
    uint64_t timestamp;
    std::vector<unsigned char> type;
    std::vector<unsigned char> user_id;
    std::vector<unsigned char> app_id;
    std::vector<unsigned char> cluster_id;
};
using amqp_basic_prop_t = struct amqp_basic_prop;

struct amqp_msg
{
    amqp_basic_prop_t properties;
    std::vector<unsigned char> body;
};
using amqp_msg_t = struct amqp_msg;

struct amqp_envlp
{
    amqp_channel_t channel;
    std::vector<unsigned char> consumer_tag;
    uint64_t delivery_tag;
    amqp_boolean_t 	redelivered;
    std::vector<unsigned char> exchange;
    std::vector<unsigned char> routing_key;
    amqp_msg_t message;
};
using amqp_envlp_t = struct amqp_envlp;




class CPPAMQP
{
    public:
        CPPAMQP(std::string h_name, int prt, int socket_create_timeout);
        CPPAMQP(const CPPAMQP&) = delete;
        CPPAMQP& operator=(const CPPAMQP&) = delete;
        CPPAMQP(CPPAMQP&&);
        CPPAMQP& operator=(CPPAMQP&&);

        ~CPPAMQP();

    void connect_normal(std::string vhost, std::string user_name, std::string password, int channel_max=0,
                        int frame_max=131072, int heartbeat=0, amqp_sasl_method_enum sasl_method=AMQP_SASL_METHOD_PLAIN);
    void close_normal();

    void connect_ssl(std::string vhost, std::string user_name, std::string password,const char *cacert,const char *cert, const char *key, int channel_max = 0,
                     int frame_max = 131072, int heartbeat=0, amqp_sasl_method_enum sasl_method=AMQP_SASL_METHOD_PLAIN);
    void close_ssl();



    void listen_to_queue(amqp_channel_t channel,const std::string& queue_name,const amqp_bytes_t& consumer_tag = amqp_empty_bytes, amqp_boolean_t no_local = 0,
                                   amqp_boolean_t no_ack = 0, amqp_boolean_t exclusive = 0,const amqp_table_t& arguments = amqp_empty_table );


    std::vector<amqp_envlp_t> receive( const int max_envs, struct timeval *tvp);
    std::vector<std::tuple<std::string,std::string,std::string>> receive_string( const int max_envs, struct timeval *tvp);
    void receive_stream(std::ostream& os, struct timeval *tvp, const std::string = "", const std::string = "");

    void send_data(const std::vector< std::vector<unsigned char>>& input_v, const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
                         const amqp_boolean_t mandatory,const amqp_boolean_t immediate);
    void send_string(const std::vector<std::string>& input_v, const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
                        const amqp_boolean_t mandatory,const amqp_boolean_t immediate);
    void send_stream(std::istream& is, const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
                        const amqp_boolean_t mandatory,const amqp_boolean_t immediate);



    protected:

    private:
    enum class RMQ_stts { DC, normal_C, SSL_C, connecting };

    void die_on_error(int x, char const *context);
    void die_on_amqp_error(amqp_rpc_reply_t x, char const *context);
    void env_copy(amqp_envlp_t& cppenv, const amqp_envelope_t& env);

    std::string hostname;
    int port;
    int timeout;
    amqp_socket_t *socket;
    amqp_connection_state_t conn;
    struct timeval tval;
    struct timeval *tv;
    std::set<std::string> q_names;
    RMQ_stts stts;
    static constexpr int SEND_STREAM_BUFFER_SIZE{8000};
    static constexpr int RECEIVE_STREAM_LOOP{8};
};


class RMQ_exception: public std::exception
{
  std::string err_text;
  public:
  RMQ_exception(std::string err):err_text{err}{};

  const char* what() const noexcept
  {
    return err_text.c_str();
  }
};

}
#endif // CPPAMQP_H
