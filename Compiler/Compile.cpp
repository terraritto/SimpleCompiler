#include "Compile.h"
#include <iostream>

Compiler::Compiler()
{
	mFile = std::make_shared<FileGenerator>();
	mTable = std::make_shared<Table>(mFile.get());
	mCodeGenerator = std::make_shared<CodeGenerator>(mTable.get(),mFile.get());
}

void Compiler::Block(int pIndex)
{
	int backP;
	backP = mCodeGenerator->GenCodeV(OpCode::jmp, 0); //内部関数を飛び越す命令、後でbackpatch

	while (true)
	{
		switch (mToken.kind)
		{
		case KeyId::Const:
			mToken = mFile->NextToken();
			ConstDecl();
			continue;
		case KeyId::Var:
			mToken = mFile->NextToken();
			VarDecl();
			continue;
		case KeyId::Func:
			mToken = mFile->NextToken();
			FuncDecl();
			continue;
		default:
			break;
		}
		break;
	}
	mCodeGenerator->BackPatch(backP); //内部関数を飛び越す命令にpatch
	mTable->ChangeV(pIndex, mCodeGenerator->NextCode()); //この関数の開始番地を修正
	mCodeGenerator->GenCodeV(OpCode::ict, mTable->FrameL()); //このブロックの実行時の必要記憶域を取る命令
	Statement(); //blockの主文
	mCodeGenerator->GenCodeR(); // return命令
	mTable->BlockEnd(); //block終了をtableに伝える
}

void Compiler::ConstDecl()
{
	//定数宣言のcompile
	Token temp;
	while (true)
	{
		if (mToken.kind == KeyId::Id)
		{
			mFile->SetIdKind(KindT::CONSTID);
			temp = mToken;
			mToken = mFile->CheckGet(mFile->NextToken(), KeyId::Equal); //名前の次は=のはず

			if (mToken.kind == KeyId::Num)
			{
				//定数名と値をtableに
				mTable->EnterTConst(temp.id, mToken.value);
			}
			else
			{
				mFile->ErrorType("number");
			}
			mToken = mFile->NextToken();
		}
		else
		{
			mFile->ErrorMissingId();
		}

		if (mToken.kind != KeyId::Comma)
		{
			//次がcommaなら定数宣言が続く
			if (mToken.kind == KeyId::Id)
			{
				//次が名前ならコンマを忘れたことにする
				mFile->ErrorInsert(KeyId::Comma);
				continue;
			}
			else
			{
				break;
			}
		}
		mToken = mFile->NextToken();
	}
	mToken = mFile->CheckGet(mToken, KeyId::Semicolon); //最後は;のはず
}

void Compiler::VarDecl()
{
	while (true)
	{
		if (mToken.kind == KeyId::Id)
		{
			mFile->SetIdKind(KindT::VARID);
			mTable->EnterTVar(mToken.id);
			mToken = mFile->NextToken();
		}
		else
		{
			mFile->ErrorMissingId();
		}

		if (mToken.kind != KeyId::Comma) //次がcommaなら変数宣言が続く
		{
			if (mToken.kind == KeyId::Id) {
				//次が名前なら忘れたことにする
				mFile->ErrorInsert(KeyId::Comma);
				continue;
			}else
			{
				break;
			}

		}
		mToken = mFile->NextToken();
	}
	mToken = mFile->CheckGet(mToken, KeyId::Semicolon); //最後は";"のはず
}

void Compiler::FuncDecl()
{
	int fIndex;
	if (mToken.kind == KeyId::Id)
	{
		mFile->SetIdKind(KindT::FUNCID);
		fIndex = mTable->EnterTFunc(mToken.id, mCodeGenerator->NextCode()); //関数名をtableに登録
		//先頭番地はまず次のコードの番地nextCode()とする
		mToken = mFile->CheckGet(mFile->NextToken(), KeyId::Lparen);
		mTable->BlockBegin(FIRSTADDR); //parameter名のレベルは関数のブロックと同じ

		while (true)
		{
			if (mToken.kind == KeyId::Id)
			{
				mFile->SetIdKind(KindT::PARID);
				mTable->EnterTPar(mToken.id);
				mToken = mFile->NextToken();
			}
			else
			{
				break;
			}

			if (mToken.kind != KeyId::Comma) //次がcommaならparameter名が続く
			{
				if (mToken.kind == KeyId::Id)
				{
					//次が名前ならcommaを忘れたことにする
					mFile->ErrorInsert(KeyId::Comma);
					continue;
				}
				else
				{
					break;
				}

			}
			mToken = mFile->NextToken();
		}

		mToken = mFile->CheckGet(mToken, KeyId::Rparen); //最後は)のはず
		mTable->EndPar();//parameter部が終わったことをtableに伝える
		if (mToken.kind == KeyId::Semicolon)
		{
			mFile->ErrorDelete();
			mToken = mFile->NextToken();
		}

		Block(fIndex); //ブロックのコンパイル,その関数名のindexを渡す
		mToken = mFile->CheckGet(mToken, KeyId::Semicolon); //最後は;のはず
	}
	else
	{
		mFile->ErrorMissingId(); //関数名がない
	}
}

void Compiler::Statement()
{
	int tIndex;
	KindT k;
	int backP, backP2; //backPatch用
	while (true)
	{
		switch (mToken.kind)
		{
		case KeyId::Id:
			//代入文
			tIndex = mTable->SearchT(mToken.id, KindT::VARID); //左辺の変数のindex
			mFile->SetIdKind(k = mTable->GetKindT(tIndex));
			if (k != KindT::VARID && k != KindT::PARID)
			{
				//変数名orパラメータ名でない
				mFile->ErrorType("var/par");
			}
			mToken = mFile->CheckGet(mFile->NextToken(), KeyId::Assign);//:=のはず
			Expression();
			mCodeGenerator->GenCodeT(OpCode::sto, tIndex); //左辺への代入命令
			return;
		case KeyId::If:
			mToken = mFile->NextToken();
			Condition();
			mToken = mFile->CheckGet(mToken, KeyId::Then); //thenのはず
			backP = mCodeGenerator->GenCodeV(OpCode::jpc, 0);
			Statement();
			mCodeGenerator->BackPatch(backP);
			return;
		case KeyId::Ret:
			mToken = mFile->NextToken();
			Expression();
			mCodeGenerator->GenCodeR();
			return;
		case KeyId::Begin:
			mToken = mFile->NextToken();
			while (true)
			{
				Statement();
				while (true)
				{
					if (mToken.kind == KeyId::Semicolon)
					{
						//次が;なら文が続く
						mToken = mFile->NextToken();
						break;
					}
					if (mToken.kind == KeyId::End)
					{
						mToken = mFile->NextToken();
						return;
					}

					if (IsStBeginKey(mToken))
					{
						//次が分の先頭記号なら;を忘れたことにする
						mFile->ErrorInsert(KeyId::Semicolon);
						break;
					}
					
					mFile->ErrorDelete();

					mToken = mFile->NextToken();
				}
			}
		case KeyId::While:
			mToken = mFile->NextToken();
			backP2 = mCodeGenerator->NextCode(); //while文の最後のjmp命令の飛び先
			Condition();
			mToken = mFile->CheckGet(mToken, KeyId::Do);//doのはず
			backP = mCodeGenerator->GenCodeV(OpCode::jpc, 0);//条件式がfalseの時飛び出すjpc命令
			Statement();
			mCodeGenerator->GenCodeV(OpCode::jmp, backP2); //while文の先頭に飛ぶジャンプ命令
			mCodeGenerator->BackPatch(backP); //falseの時に飛び出すjpc命令へのbackpatch
			return;
		case KeyId::Write:
			mToken = mFile->NextToken();
			Expression();
			mCodeGenerator->GenCodeO(Operator::wrt); //出力
			return;
		case KeyId::WriteIn:
			mToken = mFile->NextToken();
			mCodeGenerator->GenCodeO(Operator::wrl); //改行出力
			return;
		case KeyId::End:
		case KeyId::Semicolon:
		case KeyId::Period:
			return;
		default:
			mFile->ErrorDelete();
			mToken = mFile->NextToken();
			continue;
		}
	}
}

void Compiler::Expression()
{
	KeyId k;
	k = mToken.kind;
	if (k == KeyId::Plus || k == KeyId::Minus)
	{
		mToken = mFile->NextToken();
		Term();
		if (k == KeyId::Minus)
		{
			mCodeGenerator->GenCodeO(Operator::neg);
		}
	}
	else
	{
		Term();
	}
	k = mToken.kind;
	while (k == KeyId::Plus || k == KeyId::Minus)
	{
		mToken = mFile->NextToken();
		Term();
		if (k == KeyId::Minus)
		{
			mCodeGenerator->GenCodeO(Operator::sub);
		}
		else
		{
			mCodeGenerator->GenCodeO(Operator::add);
		}
		k = mToken.kind;
	}
}

void Compiler::Term()
{
	KeyId k;
	Factor();
	k = mToken.kind;
	while (k == KeyId::Mult || k == KeyId::Div)
	{
		mToken = mFile->NextToken();
		Factor();
		if (k == KeyId::Mult)
		{
			mCodeGenerator->GenCodeO(Operator::mul);
		}
		else
		{
			mCodeGenerator->GenCodeO(Operator::div);
		}
		k = mToken.kind;
	}
}

void Compiler::Factor()
{
	int tIndex, i;
	KindT k;
	
	if (mToken.kind == KeyId::Id)
	{
		tIndex = mTable->SearchT(mToken.id, KindT::VARID);
		mFile->SetIdKind(k = mTable->GetKindT(tIndex));
		switch (k)
		{
		case KindT::VARID:
		case KindT::PARID:
			mCodeGenerator->GenCodeT(OpCode::lod, tIndex);
			mToken = mFile->NextToken();
			break;
		case KindT::CONSTID:
			mCodeGenerator->GenCodeV(OpCode::lit, mTable->GetVal(tIndex));
			mToken = mFile->NextToken();
			break;
		case KindT::FUNCID:
			mToken = mFile->NextToken();
			if (mToken.kind == KeyId::Lparen)
			{
				i = 0;
				mToken = mFile->NextToken();
				if (mToken.kind != KeyId::Rparen)
				{
					while (true)
					{
						Expression();
						i++;
						if (mToken.kind == KeyId::Comma)
						{
							//次がcommaなら実引数が続く
							mToken = mFile->NextToken();
							continue;
						}
						mToken = mFile->CheckGet(mToken, KeyId::Rparen);
						break;
					}
				}
				else
				{
					mToken = mFile->NextToken();
				}

				if (mTable->GetPars(tIndex) != i)
				{
					mFile->ErrorMessage("unmatched par");
				}
			}
			else
			{
				mFile->ErrorInsert(KeyId::Lparen);
				mFile->ErrorInsert(KeyId::Rparen);
			}
			mCodeGenerator->GenCodeT(OpCode::cal, tIndex);
			break;

		}
	}
	else if (mToken.kind == KeyId::Num)
	{
		mCodeGenerator->GenCodeV(OpCode::lit, mToken.value);
		mToken = mFile->NextToken();
	}
	else if (mToken.kind == KeyId::Lparen)
	{
		mToken = mFile->NextToken();
		Expression();
		mToken = mFile->CheckGet(mToken, KeyId::Rparen);
	}

	switch (mToken.kind)
	{
	case KeyId::Id:
	case KeyId::Num:
	case KeyId::Lparen:
		mFile->ErrorMissingOp();
		Factor();
	default:
		return;
	}
}

void Compiler::Condition()
{
	KeyId k;
	if (mToken.kind == KeyId::Odd)
	{
		mToken = mFile->NextToken();
		Expression();
		mCodeGenerator->GenCodeO(Operator::odd);
	}
	else
	{
		Expression();
		k = mToken.kind;
		switch (k)
		{
		case KeyId::Equal:
		case KeyId::Lss:
		case KeyId::Gtr:
		case KeyId::NotEq:
		case KeyId::LssEq:
		case KeyId::GtrEq:
			break;
		default:
			mFile->ErrorType("rel-op");
			break;
		}

		mToken = mFile->NextToken();
		Expression();
		switch (k)
		{
		case KeyId::Equal:
			mCodeGenerator->GenCodeO(Operator::eq);
			break;
		case KeyId::Lss:
			mCodeGenerator->GenCodeO(Operator::ls);
			break;
		case KeyId::Gtr:
			mCodeGenerator->GenCodeO(Operator::gr);
			break;
		case KeyId::NotEq:
			mCodeGenerator->GenCodeO(Operator::neq);
			break;
		case KeyId::LssEq:
			mCodeGenerator->GenCodeO(Operator::lseq);
			break;
		case KeyId::GtrEq:
			mCodeGenerator->GenCodeO(Operator::greq);
			break;
		}
	}
}

int Compiler::IsStBeginKey(Token t)
{
	switch (t.kind)
	{
	case KeyId::If:
	case KeyId::Begin:
	case KeyId::Ret:
	case KeyId::While:
	case KeyId::Write:
	case KeyId::WriteIn:
		return 1;
	default:
		return 0;
	}
}

int Compiler::Compile()
{
	int i = 0;
	std::cout << "start compilation" << std::endl;
	mFile->InitSource(); //get source の初期化
	mToken = mFile->NextToken(); //最初のtoken
	mTable->BlockBegin(FIRSTADDR);

	//これ以降は新しいblockのもの
	Block(0); //0はダミー
	mFile->FinalSource();
	i = mFile->ErrorN();
	if (i != 0)
	{
		//errorが存在
		std::cout << "errors: " << i << std::endl;
	}

	//目的コードのlist
	mCodeGenerator->ListCode();

	return i < MINERROR;
}
