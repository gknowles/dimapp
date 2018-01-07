// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// httpint.h - dim http
#pragma once

#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Http connection
*
***/

const unsigned kDefaultWindowSize = 65'535;

struct HttpStream {
    enum State {
        kIdle,
        kLocalReserved,
        kRemoteReserved,
        kOpen,
        kLocalClosed,
        kRemoteClosed,
        kReset, // sent RST_STREAM, not yet confirmed
        kClosed,
        kDeleted, // waiting for garbage collection
    };

    State m_state{kIdle};
    TimePoint m_closed;
    std::unique_ptr<HttpMsg> m_msg;
    int m_flowWindow{kDefaultWindowSize};
};

class HttpConn : public HandleContent {
public:
    enum FrameFlags : uint8_t;
    enum class FrameError : int;

public:
    HttpConn();

    // Initialize as an outgoing connection, must be first method called
    // on outgoing connections after construction.
    void connect(CharBuf * out, HttpConnHandle hc);

    void accept(HttpConnHandle hc);

    // Returns false when no more data will be accepted, either by request
    // of the input or due to error.
    // Even after an error, out and msgs should be processed.
    //  - out: data to send to the remote endpoint is appended
    //  - msgs: zero or more requests, push promises, and/or replies are
    //  appended
    bool recv(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const void * src,
        size_t srcLen
    );

    // Serializes a request and returns the stream id used
    int request(CharBuf * out, const HttpMsg & msg, bool more);

    // Serializes a push promise
    int pushPromise(CharBuf * out, const HttpMsg & msg, bool more);

    // Serializes a reply on the specified stream
    void reply(CharBuf * out, int stream, const HttpMsg & msg, bool more);

    // Serializes additional data on the stream
    template<typename T>
    void addData(
        CharBuf * out,
        int stream,
        HttpStream * strm,
        const T & data,
        bool more
    );

    void resetStream(CharBuf * out, int stream);

    void deleteStream(int stream, HttpStream * sm);

private:
    enum class ByteMode : int;
    enum class FrameMode : int;

    void replyRstStream(CharBuf * out, int stream, FrameError error);
    HttpStream * findAlways(CharBuf * out, int stream);
    void writeMsg(
        CharBuf * out,
        int stream,
        HttpStream * strm,
        const HttpMsg & msg,
        bool more
    );

    bool onFrame(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[]
    );
    bool onContinuation(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onData(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onGoAway(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onHeaders(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onPing(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onPriority(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onPushPromise(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onRstStream(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onSettings(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );
    bool onWindowUpdate(
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        CharBuf * out,
        const char src[],
        int stream,
        FrameFlags flags
    );

    HttpConnHandle m_handle;
    bool m_outgoing{false};

    // byte parsing
    ByteMode m_byteMode;
    int m_inputPos{0};
    std::vector<char> m_input;
    int m_inputFrameLen{0};
    int m_maxInputFrame{16384};

    // frame parsing
    int m_lastInputStream{0};
    FrameMode m_frameMode;
    int m_continueStream{0};

    int m_nextOutputStream{0};
    int m_lastOutputStream{0};
    int m_maxOutputFrame{16384};
    int m_unackSettings{0};
    int m_flowWindow{kDefaultWindowSize};

    std::unordered_map<int, std::shared_ptr<HttpStream>> m_streams;
    HpackEncode m_encoder;
    HpackDecode m_decoder;
};

} // namespace
