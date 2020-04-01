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
	backP = mCodeGenerator->GenCodeV(OpCode::jmp, 0); //�����֐����щz�����߁A���backpatch

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
	mCodeGenerator->BackPatch(backP); //�����֐����щz�����߂�patch
	mTable->ChangeV(pIndex, mCodeGenerator->NextCode()); //���̊֐��̊J�n�Ԓn���C��
	mCodeGenerator->GenCodeV(OpCode::ict, mTable->FrameL()); //���̃u���b�N�̎��s���̕K�v�L�������閽��
	Statement(); //block�̎啶
	mCodeGenerator->GenCodeR(); // return����
	mTable->BlockEnd(); //block�I����table�ɓ`����
}

void Compiler::ConstDecl()
{
	//�萔�錾��compile
	Token temp;
	while (true)
	{
		if (mToken.kind == KeyId::Id)
		{
			mFile->SetIdKind(KindT::CONSTID);
			temp = mToken;
			mToken = mFile->CheckGet(mFile->NextToken(), KeyId::Equal); //���O�̎���=�̂͂�

			if (mToken.kind == KeyId::Num)
			{
				//�萔���ƒl��table��
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
			//����comma�Ȃ�萔�錾������
			if (mToken.kind == KeyId::Id)
			{
				//�������O�Ȃ�R���}��Y�ꂽ���Ƃɂ���
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
	mToken = mFile->CheckGet(mToken, KeyId::Semicolon); //�Ō��;�̂͂�
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

		if (mToken.kind != KeyId::Comma) //����comma�Ȃ�ϐ��錾������
		{
			if (mToken.kind == KeyId::Id) {
				//�������O�Ȃ�Y�ꂽ���Ƃɂ���
				mFile->ErrorInsert(KeyId::Comma);
				continue;
			}else
			{
				break;
			}

		}
		mToken = mFile->NextToken();
	}
	mToken = mFile->CheckGet(mToken, KeyId::Semicolon); //�Ō��";"�̂͂�
}

void Compiler::FuncDecl()
{
	int fIndex;
	if (mToken.kind == KeyId::Id)
	{
		mFile->SetIdKind(KindT::FUNCID);
		fIndex = mTable->EnterTFunc(mToken.id, mCodeGenerator->NextCode()); //�֐�����table�ɓo�^
		//�擪�Ԓn�͂܂����̃R�[�h�̔ԒnnextCode()�Ƃ���
		mToken = mFile->CheckGet(mFile->NextToken(), KeyId::Lparen);
		mTable->BlockBegin(FIRSTADDR); //parameter���̃��x���͊֐��̃u���b�N�Ɠ���

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

			if (mToken.kind != KeyId::Comma) //����comma�Ȃ�parameter��������
			{
				if (mToken.kind == KeyId::Id)
				{
					//�������O�Ȃ�comma��Y�ꂽ���Ƃɂ���
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

		mToken = mFile->CheckGet(mToken, KeyId::Rparen); //�Ō��)�̂͂�
		mTable->EndPar();//parameter�����I��������Ƃ�table�ɓ`����
		if (mToken.kind == KeyId::Semicolon)
		{
			mFile->ErrorDelete();
			mToken = mFile->NextToken();
		}

		Block(fIndex); //�u���b�N�̃R���p�C��,���̊֐�����index��n��
		mToken = mFile->CheckGet(mToken, KeyId::Semicolon); //�Ō��;�̂͂�
	}
	else
	{
		mFile->ErrorMissingId(); //�֐������Ȃ�
	}
}

void Compiler::Statement()
{
	int tIndex;
	KindT k;
	int backP, backP2; //backPatch�p
	while (true)
	{
		switch (mToken.kind)
		{
		case KeyId::Id:
			//�����
			tIndex = mTable->SearchT(mToken.id, KindT::VARID); //���ӂ̕ϐ���index
			mFile->SetIdKind(k = mTable->GetKindT(tIndex));
			if (k != KindT::VARID && k != KindT::PARID)
			{
				//�ϐ���or�p�����[�^���łȂ�
				mFile->ErrorType("var/par");
			}
			mToken = mFile->CheckGet(mFile->NextToken(), KeyId::Assign);//:=�̂͂�
			Expression();
			mCodeGenerator->GenCodeT(OpCode::sto, tIndex); //���ӂւ̑������
			return;
		case KeyId::If:
			mToken = mFile->NextToken();
			Condition();
			mToken = mFile->CheckGet(mToken, KeyId::Then); //then�̂͂�
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
						//����;�Ȃ當������
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
						//�������̐擪�L���Ȃ�;��Y�ꂽ���Ƃɂ���
						mFile->ErrorInsert(KeyId::Semicolon);
						break;
					}
					
					mFile->ErrorDelete();

					mToken = mFile->NextToken();
				}
			}
		case KeyId::While:
			mToken = mFile->NextToken();
			backP2 = mCodeGenerator->NextCode(); //while���̍Ō��jmp���߂̔�ѐ�
			Condition();
			mToken = mFile->CheckGet(mToken, KeyId::Do);//do�̂͂�
			backP = mCodeGenerator->GenCodeV(OpCode::jpc, 0);//��������false�̎���яo��jpc����
			Statement();
			mCodeGenerator->GenCodeV(OpCode::jmp, backP2); //while���̐擪�ɔ�ԃW�����v����
			mCodeGenerator->BackPatch(backP); //false�̎��ɔ�яo��jpc���߂ւ�backpatch
			return;
		case KeyId::Write:
			mToken = mFile->NextToken();
			Expression();
			mCodeGenerator->GenCodeO(Operator::wrt); //�o��
			return;
		case KeyId::WriteIn:
			mToken = mFile->NextToken();
			mCodeGenerator->GenCodeO(Operator::wrl); //���s�o��
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
							//����comma�Ȃ������������
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
	mFile->InitSource(); //get source �̏�����
	mToken = mFile->NextToken(); //�ŏ���token
	mTable->BlockBegin(FIRSTADDR);

	//����ȍ~�͐V����block�̂���
	Block(0); //0�̓_�~�[
	mFile->FinalSource();
	i = mFile->ErrorN();
	if (i != 0)
	{
		//error������
		std::cout << "errors: " << i << std::endl;
	}

	//�ړI�R�[�h��list
	mCodeGenerator->ListCode();

	return i < MINERROR;
}
