#include "file.h"
#include <cassert>
#include <cstdlib>
#include <cctype>
#include "gvb.h"

using namespace std;
using namespace gvbsim;


const char *File::toString(Mode mode) {
	switch (mode) {
	case Mode::INPUT:
		return "INPUT";
	case Mode::OUTPUT:
		return "OUTPUT";
	case Mode::RANDOM:
		return "RANDOM";
	case Mode::APPEND:
		return "APPEND";
	default:
		assert(0);
	}
}


File::File(): m_fp(nullptr) {
}

File::~File() {
	close();
}

void File::close() {
	if (m_fp) {
		fclose(m_fp);
		m_fp = nullptr;
	}
}

bool File::eof() {
	return feof(m_fp) != 0;
}

size_t File::size() {
	auto org = ftell(m_fp);
	fseek(m_fp, 0, SEEK_END);
	auto sz = ftell(m_fp);
	fseek(m_fp, org, SEEK_SET);
	return static_cast<size_t>(sz);
}

bool File::open(const string &fn, Mode mode) {
	switch (mode) {
	case Mode::RANDOM:
		m_fp = fopen(fn.data(), "rb+");
		if (!isOpen()) {
			m_fp = fopen(fn.data(), "wb");
			if (isOpen()) {
				fclose(m_fp);
				m_fp = fopen(fn.data(), "rb+");
			}
		}
		break;
	case Mode::INPUT:
		m_fp = fopen(fn.data(), "rb");
		break;
	case Mode::OUTPUT:
		m_fp = fopen(fn.data(), "wb");
		break;
	case Mode::APPEND:
		m_fp = fopen(fn.data(), "ab");
		break;
	default:
		assert(0);
	}
	return isOpen();
}

void File::ungetc(char c) {
	std::ungetc(c, m_fp);
}

void File::skipOneByte() {
	readByte();
}

void File::seek(long pos) {
	fseek(m_fp, pos, SEEK_SET);
}

void File::writeByte(char c) {
	putc(c, m_fp);
}

void File::writeString(const string &s) {
	fwrite(s.data(), s.size(), 1, m_fp);
}

void File::writeReal(double r) {
	char buf[50];
	fwrite(buf, sprintf(buf, "%.9G", r), 1, m_fp);
}

char File::readByte() {
	return static_cast<char>(fgetc(m_fp));
}

bool File::readContent(string &s) {
	int c = fgetc(m_fp);

	if ('"' == c) {
		while ((c = fgetc(m_fp)) != '"' && c != -1)
			s += static_cast<char>(c);
		return true;
	} else {
		while (c != ',' && c != 0xff && c != -1) {
			s += static_cast<char>(c);
			c = fgetc(m_fp);
		}
		if (c != -1)
			this->ungetc(static_cast<char>(c));
		return false;
	}
}

string File::readString() {
	string buf;
	readContent(buf);
	return buf;
}

double File::readReal() {
	string buf;
	if (readContent(buf))
		return 0.;

   return GVB::str2d(buf);
}