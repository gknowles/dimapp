// Copyright Glen Knowles 2016 - 2022.
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
        kReserved,
        kOpen,
        kClosed,
        kDeleted, // waiting for garbage collection
    };

    State m_localState{kIdle};
    State m_remoteState{kIdle};
    TimePoint m_closed;
    std::unique_ptr<HttpMsg> m_msg;
    int m_flowWindow{0};
    CharBuf m_unsent;

    HttpStream();
    ~HttpStream();
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
    //  - out: data to send to the remote address is appended
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
    bool reply(CharBuf * out, int stream, const HttpMsg & msg, bool more);

    // Serializes additional data on the stream
    template<typename T>
    bool addData(
        CharBuf * out,
        int stream,
        HttpStream * strm,
        const T & data,
        bool more
    );

    void resetStream(CharBuf * out, int stream, FrameError error);

    void deleteStream(int stream, HttpStream * sm);

    std::string_view errmsg() const { return m_errmsg; }

private:
    enum class ByteMode : int;
    enum class FrameMode : int;

    // Always returns false
    bool replyGoAway(
        Dim::CharBuf * out,
        int lastStream,
        HttpConn::FrameError error,
        std::string_view msg
    );
    // Always returns false, uses m_lastInputStream in go away message
    bool replyGoAway(
        Dim::CharBuf * out,
        HttpConn::FrameError error,
        std::string_view msg
    );

    HttpStream * findAlways(CharBuf * out, int stream);
    bool writeMsg(
        CharBuf * out,
        int stream,
        HttpStream * strm,
        const HttpMsg & msg,
        bool more
    );
    void writeUnsent(CharBuf * out, int stream, HttpStream * strm);
    bool setInitialWindowSize(CharBuf * out, unsigned value);

    bool onFrame(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[]
    );
    bool onContinuation(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onData(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onGoAway(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onHeaders(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onPing(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onPriority(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onPushPromise(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onRstStream(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onSettings(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
    );
    bool onWindowUpdate(
        CharBuf * out,
        std::vector<std::unique_ptr<HttpMsg>> * msgs,
        const char src[],
        int stream,
        EnumFlags<FrameFlags> flags
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
    int m_initialFlowWindow{kDefaultWindowSize};

    std::unordered_map<int, std::shared_ptr<HttpStream>> m_streams;
    HpackEncode m_encoder;
    HpackDecode m_decoder;
    std::string m_errmsg;
};

} // namespace
