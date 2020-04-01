#include "GetSource.h"
#include <iostream>

FileGenerator::FileGenerator()
	: mErrorNo(0)
{
	// construct KeyWd
	mKeyWdT.push_back(keyWd("begin", KeyId::Begin));
	mKeyWdT.push_back(keyWd("end", KeyId::End));
	mKeyWdT.push_back(keyWd("if", KeyId::If));
	mKeyWdT.push_back(keyWd("then", KeyId::Then));
	mKeyWdT.push_back(keyWd("while", KeyId::While));
	mKeyWdT.push_back(keyWd("do", KeyId::Do));
	mKeyWdT.push_back(keyWd("return", KeyId::Ret));
	mKeyWdT.push_back(keyWd("function", KeyId::Func));
	mKeyWdT.push_back(keyWd("var", KeyId::Var));
	mKeyWdT.push_back(keyWd("const", KeyId::Const));
	mKeyWdT.push_back(keyWd("odd", KeyId::Odd));
	mKeyWdT.push_back(keyWd("write", KeyId::Write));
	mKeyWdT.push_back(keyWd("writeln", KeyId::WriteIn));
	mKeyWdT.push_back(keyWd("@dummy1", KeyId::end_of_KeyWd));
	mKeyWdT.push_back(keyWd("+", KeyId::Plus));
	mKeyWdT.push_back(keyWd("-", KeyId::Minus));
	mKeyWdT.push_back(keyWd("*", KeyId::Mult));
	mKeyWdT.push_back(keyWd("/", KeyId::Div));
	mKeyWdT.push_back(keyWd("(", KeyId::Lparen));
	mKeyWdT.push_back(keyWd(")", KeyId::Rparen));
	mKeyWdT.push_back(keyWd("=", KeyId::Equal));
	mKeyWdT.push_back(keyWd("<", KeyId::Lss));
	mKeyWdT.push_back(keyWd(">", KeyId::Gtr));
	mKeyWdT.push_back(keyWd("<>", KeyId::NotEq));
	mKeyWdT.push_back(keyWd("<=", KeyId::LssEq));
	mKeyWdT.push_back(keyWd(">=", KeyId::GtrEq));
	mKeyWdT.push_back(keyWd(",", KeyId::Comma));
	mKeyWdT.push_back(keyWd(",", KeyId::Period));
	mKeyWdT.push_back(keyWd(";", KeyId::Semicolon));
	mKeyWdT.push_back(keyWd(":=", KeyId::Assign));
	mKeyWdT.push_back(keyWd("@dummy2", KeyId::end_of_KeySym));
}

Token FileGenerator::NextToken()
{
	int i = 0;
	int num;
	KeyId cc;
	Token temp;
	char ident[MAXNAME];
	
	PrintCToken(); //前のtokenを印字

	mSpaces = 0; mCR = 0;
	while (true)
	{
		// 次のトークンまでの空白や改行をカウント
		if (mCh == ' ')
		{
			mSpaces++;
		}
		else if (mCh == '\t')
		{
			mSpaces += TAB;
		}
		else if (mCh == '\n')
		{
			mSpaces = 0; mCR++;
		}
		else if (mCh == '\0')
		{
			mCh = NextChar();
			continue;
		}
		else break;
		mCh = NextChar();
	}

	switch (cc = charClassT[mCh])
	{
	case KeyId::letter:
		do {
			if (i < MAXNAME)
			{
				ident[i] = mCh;
			}
			i++; mCh = NextChar();
		} while (
			charClassT[mCh] == KeyId::letter
			|| charClassT[mCh] == KeyId::digit);
		
		if (i >= MAXNAME)
		{
			ErrorMessage("too long");
			i = MAXNAME - 1;
		}
		ident[i] = '\0';

		for (i = 0; i < static_cast<int>(KeyId::end_of_KeyWd); i++)
		{
			//予約語の場合
			if (ident == mKeyWdT[i].word)
			{
				temp.kind = mKeyWdT[i].keyId;
				mCToken = temp; mPrinted = 0;
				return temp;
			}
		}

		temp.kind = KeyId::Id; // user 宣言の名前
		temp.id = ident;
		break;
	case KeyId::digit:
		num = 0;
		do {
			num = 10 * num + (mCh - '0');
			i++; mCh = NextChar();
		} while (charClassT[mCh] == KeyId::digit);

		if (i > MAXNUM)
		{
			ErrorMessage("too large");
		}

		temp.kind = KeyId::Num;
		temp.value = num;
		break;
	case KeyId::colon:
		if ((mCh = NextChar()) == '=') // :=
		{
			mCh = NextChar();
			temp.kind = KeyId::Assign;
			break;
		}
		else
		{
			temp.kind = KeyId::nul;
			break;
		}
	case KeyId::Lss:
		if ((mCh = NextChar()) == '=')
		{
			mCh = NextChar();
			temp.kind = KeyId::LssEq; // <=
			break;
		}
		else if (mCh == '>')
		{
			mCh = NextChar();
			temp.kind = KeyId::NotEq; // <>
			break;
		}
		else
		{
			temp.kind = KeyId::Lss; // <
			break;
		}
	case KeyId::Gtr:
		if ((mCh = NextChar()) == '=')
		{
			mCh = NextChar(); // >=
			temp.kind = KeyId::GtrEq;
			break;
		}
		else
		{
			temp.kind = KeyId::Gtr; // >
			break;
		}
	default:
		temp.kind = cc;
		mCh = NextChar(); 
		break;
	}
	mCToken = temp; mPrinted = 0;
	return temp;
}

Token FileGenerator::CheckGet(Token t, KeyId k)
{
	if (t.kind == k)
	{
		return NextToken(); //次のトークンを読んで返す
	}

	if (
		(IsKeyWd(k) && IsKeyWd(t.kind))
		|| (IsKeySym(k) && IsKeySym(t.kind))
		)//予約語
	{
		//tをkで置き換える
		ErrorDelete();
		ErrorInsert(k);
		return NextToken();
	}

	ErrorInsert(k);
	return t;
}

bool FileGenerator::OpenSource(std::string fileName)
{
	std::string fileNameO;

	// file open
	mFpi.open(fileName, std::ios::in);
	if (mFpi.is_open() == false)
	{
		std::cout << "can't open file: " << fileName << std::endl;
		return false;
	}

	// output filename
	fileNameO = fileName + ".tex";
	mFptex.open(fileNameO, std::ios::out);
	if (mFptex.is_open() == false)
	{
		std::cout << "can't open fi;e: " << fileNameO << std::endl;
		return false;
	}

	return true;
}

void FileGenerator::CloseSource()
{
	mFpi.close();
	mFptex.close();
}

void FileGenerator::InitSource()
{
	// initialize
	mLineIndex = -1; 
	mCh = '\n';
	mPrinted = 1;
	InitCharClassT();

	// LaTeX command
	mFptex << "\\documentstyle[12pt]{article}" << std::endl;
	mFptex << "\\begin{document}" << std::endl;
	mFptex << "\\fboxsep=0pt" << std::endl;
	mFptex << "\\def\\insert#1{$\\fbox{#1}$}" << std::endl;
	mFptex << "\\def\\delete#1{$\\fboxrule=.5mm\\fbox{#1}$}" << std::endl;
	mFptex << "\\rm" << std::endl;
}

void FileGenerator::FinalSource()
{
	if (mCToken.kind == KeyId::Period)
	{
		PrintCToken();
	}
	else
	{
		ErrorInsert(KeyId::Period);
	}

	mFptex << std::endl << "\\end{document}" << std::endl;
}

void FileGenerator::Error(std::string m) // output error
{
	if (mLineIndex > 0)
	{
		std::cout << "line:" << mLineIndex << std::endl;
	}
	std::cout << "*** error *** : " << m << std::endl;
	mErrorNo++;
	if (mErrorNo > MAXERROR)
	{
		std::cout << "too many errors" << std::endl;
		std::cout << "abort compilation" << std::endl;
		exit(1);
	}
}

void FileGenerator::ErrorNoCheck() // count error
{
	if (mErrorNo > MAXERROR)
	{
		mFptex << "too many errors" << std::endl;
		mFptex << "\\end{document}" << std::endl;
		std::cout << "abort compilation" << std::endl;
		exit(1);
	}

}

void FileGenerator::ErrorType(std::string m) // type error
{
	PrintSpaces();
	mFptex << "\\(\\stackrel{\\mbox{\\scriptsize " << m << "}}{\\mbox{";
	PrintCToken();
	mFptex << "}}\\)";
	ErrorNoCheck();
}

void FileGenerator::ErrorInsert(KeyId k) // insert KeyString(k)
{
	if (k < KeyId::end_of_KeyWd)
	{
		mFptex << "\\ \\insert{{\\bf " << mKeyWdT[static_cast<int>(k)].word << "}}";
	}
	else
	{
		mFptex << "\\ \\insert{$" << mKeyWdT[static_cast<int>(k)].word << "$}";
	}
	ErrorNoCheck();
}

void FileGenerator::ErrorMissingId() // not exist name error
{
	mFptex << "\\insert{Id}";
	ErrorNoCheck();
}

void FileGenerator::ErrorMissingOp() // not exist operator error
{
	mFptex << "\\insert{$\\otimes$}";
	ErrorNoCheck();
}

void FileGenerator::ErrorDelete()
{
	int i = static_cast<int>(mCToken.kind);
	mPrinted = 1;

	if (i < static_cast<int>(KeyId::end_of_KeyWd))
	{
		mFptex << "\\delete{{\\bf " << mKeyWdT[i].word << "}}";
	}
	else if (i < static_cast<int>(KeyId::end_of_KeySym))
	{
		mFptex << "\\delete{$" << mKeyWdT[i].word << "$}";
	}
	else if (i == static_cast<int>(KeyId::Id))
	{
		mFptex << "\\delete{" << mCToken.id << "}";
	}
	else if (i == static_cast<int>(KeyId::Num))
	{
		mFptex << "\\delete{" << mCToken.value << "}";
	}
}

void FileGenerator::ErrorMessage(std::string m)
{
	mFptex << "$^{" << m << "}$";
	ErrorNoCheck();
}

void FileGenerator::ErrorF(std::string m)
{
	ErrorMessage(m);
	mFptex << "fatal errors" << std::endl;
	mFptex << "\\end{document}" << std::endl;

	if (mErrorNo) {
		std::cout << "total errors: " << mErrorNo << std::endl;
	}
	std::cout << "abort compilation" << std::endl;
	exit(1);
}

int FileGenerator::ErrorN()
{
	return mErrorNo;
}

void FileGenerator::SetIdKind(KindT k)
{
	mIdKind = k;
}

char FileGenerator::NextChar()
{
	char ch;
	if (mLineIndex == -1)
	{
		if (std::getline(mFpi, mLine))
		{
			std::cout << mLine << std::endl;

			mLineIndex = 0;
		}
		else
		{
			ErrorF("end of file\n");
		}
	}

	if ((ch = mLine[mLineIndex++]) == '\0') // chに次の文字を入れる
	{
		mLineIndex = -1; //改行なら次の行
		return '\n';
	}

	return ch;
}

bool FileGenerator::IsKeySym(KeyId k)
{
	if (static_cast<int>(k) < static_cast<int>(KeyId::end_of_KeyWd))
	{
		return false;
	}

	return (static_cast<int>(k) < static_cast<int>(KeyId::end_of_KeySym));
}

bool FileGenerator::IsKeyWd(KeyId k)
{
	return (static_cast<int>(k) < static_cast<int>(KeyId::end_of_KeyWd));
}

void FileGenerator::InitCharClassT()
{
	for (auto& key : charClassT)
	{
		key = KeyId::others;
	}

	for (int i = '0'; i < '9'; i++)
	{
		charClassT[i] = KeyId::digit;
	}

	for (int i = 'A'; i <= 'Z'; i++)
	{
		charClassT[i] = KeyId::letter;
	}

	for (int i = 'a'; i <= 'z'; i++)
	{
		charClassT[i] = KeyId::letter;
	}

	charClassT['+'] = KeyId::Plus; charClassT['-'] = KeyId::Minus;
	charClassT['*'] = KeyId::Mult; charClassT['/'] = KeyId::Div;
	charClassT['('] = KeyId::Lparen; charClassT[')'] = KeyId::Rparen;
	charClassT['='] = KeyId::Equal; charClassT['<'] = KeyId::Lss;
	charClassT['>'] = KeyId::Gtr; charClassT[','] = KeyId::Comma;
	charClassT['.'] = KeyId::Comma; charClassT[';'] = KeyId::Semicolon;
	charClassT[':'] = KeyId::colon;
}

void FileGenerator::PrintSpaces()
{
	while (mCR-- > 0)
	{
		mFptex << "\\ \\par" << std::endl;
	}
	while (mSpaces-- > 0)
	{
		mFptex << "\\ ";
	}
	mCR = 0; mSpaces = 0;
}

void FileGenerator::PrintCToken()
{
	int i = static_cast<int>(mCToken.kind);
	if (mPrinted)
	{
		mPrinted = 0; return;
	}
	mPrinted = 1;
	PrintSpaces();
	if (i < static_cast<int>(KeyId::end_of_KeyWd)) //予約語
	{
		mFptex << "{\\bf " << mKeyWdT[i].word << "}";
	}
	else if (i < static_cast<int>(KeyId::end_of_KeySym)) // operator
	{
		mFptex << "$" << mKeyWdT[i].word << "$";
	}
	else if (i == static_cast<int>(KeyId::Id))
	{
		switch (mIdKind)
		{
		case KindT::VARID:
			mFptex << mCToken.id;
			return;
		case KindT::PARID:
			mFptex << "{\\sl " << mCToken.id << "}";
			return;
		case KindT::FUNCID:
			mFptex << "{\\it " << mCToken.id << "}";
			return;
		case KindT::CONSTID:
			mFptex << "{\\sf " << mCToken.id << "}";
			return;
		}
	}
	else if (i == static_cast<int>(KeyId::Num))
	{
		mFptex << mCToken.value;
	}
}
