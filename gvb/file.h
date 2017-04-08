#ifndef GVBASIC_FILE_H
#define GVBASIC_FILE_H

#include <cstdio>
#include <string>

namespace gvbsim {


	class File {
	public:
		enum class Mode {
			INPUT, OUTPUT, APPEND, RANDOM
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
		void skipOneByte();
		void seek(long);

		void writeByte(char);
		void writeString(const std::string &);
		void writeReal(double);

		char readByte();
		double readReal(); // 返回的double未经过validate
		std::string readString();

	private:
		void ungetc(char);
		bool readContent(std::string &); // 返回是否有双引号

	public:
		static const char *toString(Mode);
	};

}

#endif //GVBASIC_FILE_H
