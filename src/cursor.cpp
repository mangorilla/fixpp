#include <fixpp/utils/cursor.h>

#include <cstring>

bool
StreamCursor::advance(size_t count) {
    if (count > buf->in_avail())
        return false;

    for (size_t i = 0; i < count; ++i) {
        buf->sbumpc();
    }

    return true;
}

bool
StreamCursor::eof() const {
    return remaining() == 0;
}

int
StreamCursor::next() const {
    if (buf->in_avail() < 1)
        return Eof;

    return buf->snext();
}

char
StreamCursor::current() const {
    return buf->sgetc();
}

const char*
StreamCursor::offset() const {
    return buf->curptr();
}

const char*
StreamCursor::offset(size_t off) const {
    return buf->begptr() + off;
}

size_t
StreamCursor::diff(size_t other) const {
    return buf->position() - other;
}

size_t
StreamCursor::diff(const StreamCursor& other) const {
    return other.buf->position() - buf->position();
}

size_t
StreamCursor::remaining() const {
    return buf->in_avail();
}

void
StreamCursor::reset() {
    buf->reset();
}

bool
match_raw(const void* buf, size_t len, StreamCursor& cursor) {
    if (cursor.remaining() < len)
        return false;

    if (std::memcmp(cursor.offset(), buf, len) == 0) {
        cursor.advance(len);
        return true;
    }

    return false;
}

bool
match_string(const char* str, size_t len, StreamCursor& cursor, CaseSensitivity cs) {
    if (cursor.remaining() < len)
        return false;

    if (cs == CaseSensitivity::Sensitive) {
        if (std::strncmp(cursor.offset(), str, len) == 0) {
            cursor.advance(len);
            return true;
        }
    } else {
        const char *off = cursor.offset();
        for (size_t i = 0; i < len; ++i) {
            const char lhs = std::tolower(str[i]);
            const char rhs = std::tolower(off[i]);
            if (lhs != rhs) return false;
        }

        cursor.advance(len);
        return true;
    }

    return false;
}

bool
match_literal(char c, StreamCursor& cursor, CaseSensitivity cs) {
    if (cursor.eof())
        return false;

    char lhs = (cs == CaseSensitivity::Sensitive ? c : std::tolower(c));
    char rhs = (cs == CaseSensitivity::Sensitive ? cursor.current() : std::tolower(cursor.current()));

    if (lhs == rhs) {
        cursor.advance(1);
        return true;
    }

    return false;
}

bool
match_until(char c, StreamCursor& cursor, CaseSensitivity cs) {
    return match_until( { c }, cursor, cs);
}

bool
match_until(std::initializer_list<char> chars, StreamCursor& cursor, CaseSensitivity cs) {
    if (cursor.eof())
        return false;

    auto find = [&](char val) {
        for (auto c: chars) {
            char lhs = cs == CaseSensitivity::Sensitive ? c : std::tolower(c);
            char rhs = cs == CaseSensitivity::Insensitive ? val : std::tolower(val);

            if (lhs == rhs) return true;
        }

        return false;
    };

    while (!cursor.eof()) {
        const char c = cursor.current();
        if (find(c)) return true;
        cursor.advance(1);
    }

    return false;
}

bool
match_double(double* val, StreamCursor &cursor) {
    // @Todo: strtod does not support a length argument
    char *end;
    *val = strtod(cursor.offset(), &end);
    if (end == cursor.offset())
        return false;

    cursor.advance(static_cast<ptrdiff_t>(end - cursor.offset()));
    return true;
}

bool
match_int(int* val, StreamCursor &cursor) {
    char *end;
    *val = strtol(cursor.offset(), &end, 10);
    if (end == cursor.offset())
        return false;

    cursor.advance(static_cast<ptrdiff_t>(end - cursor.offset()));
    return true;
}

void
skip_whitespaces(StreamCursor& cursor) {
    if (cursor.eof())
        return;

    int c;
    while ((c = cursor.current()) != StreamCursor::Eof && (c == ' ' || c == '\t')) {
        cursor.advance(1);
    }
}