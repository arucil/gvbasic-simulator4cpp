#ifndef GVBASIC_FILE_H
#define GVBASIC_FILE_H

#include <cstdio>
#include <string>

namespace gvbsim {


	class File {
	public:
		enum class Mode {
			INPUT, OUTPUT, APPEND, RANDOM, BINARY
		};

	private:
		std::FILE *m_fp;
		Mode m_mode;

	public:
		File();
		~File();

	public:
		bool isOpen() const { return m_fp != nullptr; }
		bool open(const std::string &, Mode);
		Mode mode() const { return m_mode; }
		bool eof();
		size_t size();
		void close();
		void skip(size_t); // 跳过n个字节
		void seek(long);
		long tell();

		void writeByte(char);
		void writeString(const std::string &);
		void writeReal(double);

		char readByte();
		double readReal(); // 返回的double未经过validate
		std::string readString();
      std::string readString(size_t); // 读取固定长度的string

	private:
		void ungetc(int);
		bool readContent(std::string &); // 返回是否有双引号

	public:
		static const char *toString(Mode);
	};

}

#endif //GVBASIC_FILE_H
