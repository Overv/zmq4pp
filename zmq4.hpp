#ifndef ZMQ4_HPP
#define ZMQ4_HPP

#include <zmq.h>

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace zmq {
    using std::string;
    using std::vector;
    using std::shared_ptr;
    using std::runtime_error;

    void zmq_check(int ret) {
        if (ret != 0) {
            throw runtime_error(zmq_strerror(zmq_errno()));
        }
    }

    enum class socket_type {
        req = ZMQ_REQ,
        rep = ZMQ_REP,
        dealer = ZMQ_DEALER,
        router = ZMQ_ROUTER,
        pub = ZMQ_PUB,
        sub = ZMQ_SUB,
        xpub = ZMQ_XPUB,
        xsub = ZMQ_XSUB,
        push = ZMQ_PUSH,
        pull = ZMQ_PULL,
        pair = ZMQ_PAIR,
        stream = ZMQ_STREAM,
    };

    enum class socket_int_option {
        sndhwm = ZMQ_SNDHWM,
        rcvhwm = ZMQ_RCVHWM,
        rate = ZMQ_RATE,
        recovery_ivl = ZMQ_RECOVERY_IVL,
        sndbuf = ZMQ_SNDBUF,
        rcvbuf = ZMQ_RCVBUF,
        linger = ZMQ_LINGER,
        reconnect_ivl = ZMQ_RECONNECT_IVL,
        reconnect_ivl_max = ZMQ_RECONNECT_IVL_MAX,
        backlog = ZMQ_BACKLOG,
        multicast_hops = ZMQ_MULTICAST_HOPS,
        rcvtimeo = ZMQ_RCVTIMEO,
        sndtimeo = ZMQ_SNDTIMEO,
        tcp_keepalive = ZMQ_TCP_KEEPALIVE,
        tcp_keepalive_idle = ZMQ_TCP_KEEPALIVE_IDLE,
        tcp_keepalive_cnt = ZMQ_TCP_KEEPALIVE_CNT,
        tcp_keepalive_intvl = ZMQ_TCP_KEEPALIVE_INTVL,
    };

    enum class socket_bool_option {
        ipv6 = ZMQ_IPV6,
        ipv4only = ZMQ_IPV4ONLY,
        immediate = ZMQ_IMMEDIATE,
        router_mandatory = ZMQ_ROUTER_MANDATORY,
        router_raw = ZMQ_ROUTER_RAW,
        probe_router = ZMQ_PROBE_ROUTER,
        xpub_verbose = ZMQ_XPUB_VERBOSE,
        req_correlate = ZMQ_REQ_CORRELATE,
        req_relaxed = ZMQ_REQ_RELAXED,
        plain_server = ZMQ_PLAIN_SERVER,
        curve_server = ZMQ_CURVE_SERVER,
        conflate = ZMQ_CONFLATE,
    };

    enum class socket_int64_option {
        maxmsgsize = ZMQ_MAXMSGSIZE,
    };

    enum class socket_uint64_option {
        affinity = ZMQ_AFFINITY,
    };

    enum class socket_string_option {
        subscribe = ZMQ_SUBSCRIBE,
        unsubscribe = ZMQ_UNSUBSCRIBE,
        identity = ZMQ_IDENTITY,
        tcp_accept_filter = ZMQ_TCP_ACCEPT_FILTER,
        plain_username = ZMQ_PLAIN_USERNAME,
        plain_password = ZMQ_PLAIN_PASSWORD,
        curve_publickey = ZMQ_CURVE_PUBLICKEY,
        curve_secretkey = ZMQ_CURVE_SECRETKEY,
        curve_serverkey = ZMQ_CURVE_SERVERKEY,
        zap_domain = ZMQ_ZAP_DOMAIN,
    };

    enum class context_option {
        io_threads = ZMQ_IO_THREADS,
        max_sockets = ZMQ_MAX_SOCKETS,
        ipv6 = ZMQ_IPV6
    };

    class socket;

    class context {
        friend class socket;

    public:
        context() noexcept {
            ctx = shared_ptr<void>(zmq_ctx_new(), [=](void* ctx) {
                zmq_ctx_term(ctx);
            });
        }

        void set(context_option option, int value) noexcept {
            zmq_ctx_set(ctx.get(), static_cast<int>(option), value);
        }

        int get(context_option option) const noexcept {
            return zmq_ctx_get(ctx.get(), static_cast<int>(option));
        }

    private:
        shared_ptr<void> ctx;
    };

    class socket {
    public:
        socket(context& context, socket_type type) noexcept {
            void* s = zmq_socket(context.ctx.get(), static_cast<int>(type));

            sock = shared_ptr<void>(s, [=](void* socket) {
                zmq_close(socket);
            });
        }

        void bind(const string& endpoint) {
            this->endpoint = endpoint;
            zmq_check(zmq_bind(sock.get(), endpoint.c_str()));
        }

        void unbind() {
            zmq_check(zmq_unbind(sock.get(), endpoint.c_str()));
        }

        void connect(const string& endpoint) {
            this->endpoint = endpoint;
            zmq_check(zmq_connect(sock.get(), endpoint.c_str()));
        }

        void disconnect() {
            zmq_check(zmq_disconnect(sock.get(), endpoint.c_str()));
        }

        void close() noexcept {
            zmq_close(sock.get());
        }

        vector<string> recv_multipart(bool blocking = true) {
            int flags = blocking ? 0 : ZMQ_DONTWAIT;

            vector<string> parts;
            zmq_msg_t part;

            do {
                zmq_msg_init(&part);

                if (zmq_msg_recv(&part, sock.get(), flags) == -1) {
                    zmq_msg_close(&part);
                    throw runtime_error(zmq_strerror(zmq_errno()));
                }

                size_t size = zmq_msg_size(&part);
                char* data = reinterpret_cast<char*>(zmq_msg_data(&part));

                string contents(data, size);
                parts.push_back(contents);

                zmq_msg_close(&part);
            } while (zmq_msg_more(&part));

            return parts;
        }

        void send_multipart(const vector<string> parts, bool blocking = true) {
            int flags = blocking ? 0 : ZMQ_DONTWAIT;

            for (int i = 0; i < parts.size(); i++) {
                int moreFlag = i == parts.size() - 1 ? 0 : ZMQ_SNDMORE;

                if (zmq_send(sock.get(), parts[i].c_str(), parts[i].size(), flags | moreFlag) == -1) {
                    throw runtime_error(zmq_strerror(zmq_errno()));
                }
            }
        }

        string recv(bool blocking = true) {
            return recv_multipart(blocking)[0];
        }

        void send(const string& msg, bool blocking = true) {
            send_multipart({msg}, blocking);
        }

        void set_opt(socket_int_option option, int value) {
            zmq_check(zmq_setsockopt(sock.get(), static_cast<int>(option), &value, sizeof(value)));
        }

        void set_opt(socket_bool_option option, bool value) {
            zmq_check(zmq_setsockopt(sock.get(), static_cast<int>(option), &value, sizeof(value)));
        }

        void set_opt(socket_int64_option option, int64_t value) {
            zmq_check(zmq_setsockopt(sock.get(), static_cast<int>(option), &value, sizeof(value)));
        }

        void set_opt(socket_uint64_option option, uint64_t value) {
            zmq_check(zmq_setsockopt(sock.get(), static_cast<int>(option), &value, sizeof(value)));
        }

        void set_opt(socket_string_option option, const string& value) {
            zmq_check(zmq_setsockopt(sock.get(), static_cast<int>(option), value.c_str(), value.size()));
        }

        void subscribe(const string& filter) {
            set_opt(socket_string_option::subscribe, filter);
        }

        void unsubscribe(const string& filter) {
            set_opt(socket_string_option::unsubscribe, filter);
        }

    private:
        string endpoint;
        shared_ptr<void> sock;
    };

    struct version_t {
        int major;
        int minor;
        int patch;
    };

    version_t version() {
        version_t v;
        zmq_version(&v.major, &v.minor, &v.patch);
        return v;
    }

    string z85_decode(const string& str) {
        // See: http://api.zeromq.org/4-0:zmq-z85-decode
        vector<char> buf(str.size() * 10 / 8);

        if (!zmq_z85_decode((uint8_t*) buf.data(), str.c_str())) {
            throw runtime_error("z85 decoding failed");
        }

        return string(buf.data(), buf.size());
    }

    string z85_encode(const string& str) {
        // See: http://api.zeromq.org/4-0:zmq-z85-encode
        vector<char> buf(str.size() * 5 / 4 + 1);

        if (!zmq_z85_encode(buf.data(), (uint8_t*) str.c_str(), str.size())) {
            throw runtime_error("z85 encoding failed");
        }

        return string(buf.data(), buf.size());
    }
}

#endif
