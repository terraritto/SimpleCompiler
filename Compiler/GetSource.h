#pragma once
#include "Table.h"
#include <fstream>
#include <string>
#include <vector>
#include <array>

constexpr int MAXLINE = 120;
constexpr int MAXERROR = 30;
constexpr int MAXNUM = 14;
constexpr int TAB = 5;

enum class KeyId
{
	Begin, End,
	If, Then,
	While, Do,
	Ret, Func,
	Var, Const, Odd,
	Write, WriteIn,
	end_of_KeyWd,
	Plus, Minus,
	Mult, Div,
	Lparen, Rparen,
	Equal, Lss, Gtr,
	NotEq, LssEq, GtrEq,
	Comma, Period, Semicolon,
	Assign,
	end_of_KeySym,
	Id, Num, nul,
	end_of_Token,
	letter, digit, colon, others
};

struct Token
{
	KeyId kind;
	std::string id;
	int value;
};

struct keyWd
{
	std::string word;
	KeyId keyId;

	keyWd(std::string w, KeyId id):word(w),keyId(id){}
};

class FileGenerator
{
public:
	FileGenerator();
	Token NextToken();
	Token CheckGet(Token t, KeyId k);
	bool OpenSource(std::string fileName);
	void CloseSource();
	void InitSource();
	void FinalSource();
	void Error(std::string m);
	void ErrorNoCheck();
	void ErrorType(std::string m);
	void ErrorInsert(KeyId k);
	void ErrorMissingId();
	void ErrorMissingOp();
	void ErrorDelete();
	void ErrorMessage(std::string m);
	void ErrorF(std::string m);
	int ErrorN();
	void SetIdKind(KindT k);

	char NextChar();
	
	bool IsKeySym(KeyId k);
	bool IsKeyWd(KeyId k);

	std::array<KeyId,256> charClassT; // •¶Žš‚ÌŽí—Þ‚ðŽ¦‚·•\
	void InitCharClassT(); // initialize

	void PrintSpaces();
	void PrintCToken();
public:
	std::ifstream mFpi;
	std::ofstream mFptex;
	std::string mLine;
	int mLineIndex;
	char mCh;

	Token mCToken;
	KindT mIdKind;
	int mSpaces;
	int mCR;
	int mPrinted;
	int mErrorNo;
	std::vector<keyWd> mKeyWdT;
};