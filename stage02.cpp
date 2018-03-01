// Brian Leary
/*
	This program is a Pascallite compiler. It is capable of compiling a 
	simple programming language into assembly code. This was the final
	project for a class over compilers and algorithmic languages. The program
	can be compiled in a Unix environment.
*/

#include <ctime>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <stack>

using namespace std;

// Maximum number of entries for the symbol table
const int MAX_SYMBOL_TABLE_SIZE = 256;

// Enums
enum storeType {INTEGER, BOOLEAN, PROG_NAME, UNKNOWN};
enum allocation {YES, NO};
enum modes {VARIABLE, CONSTANT};

// Define symbol table entry format
struct entry
{
	string internalName;
	string externalName;
	storeType dataType;
	modes mode;
	string value;
	allocation alloc;
	int units;
};

// Vector holding the symbol table
vector<entry> symbolTable;

// Input file
ifstream sourceFile;

// List and object output files
ofstream listingFile, objectFile;

// Variables that might be used by any of the functions
string token;
bool printLineNumber = false;
char charac;
const char END_OF_FILE = '$'; // arbitrary choice
unsigned int lineNumber = 0;
unsigned int integerCount = 0;
unsigned int booleanCount = 0;

// Stage 1
stack<string> operandStk;
stack<string> operatorStk;
string currentARegister = "";
int currentTempNo = -1;
int currentLabelNo = -1;
int maxTempNo = -1;

// Function prototypes (stage 0)
void CreateListingHeader();
void Parser();
void CreateListingTrailer();
void Prog();
void ProgStmt();
void Consts();
void Vars();
void BeginEndStmt(bool internalBeginEnd);
void ConstStmts();
void VarStmts();
string Ids();
void Insert(string externalName, storeType inType, modes inMode, string inValue, allocation inAlloc, int inUnits);
storeType WhichType(string name);
string WhichValue(string name);
string NextToken();
char NextChar();
void Error(string errorMessage);
bool CheckNonKeyID(string currentToken);
bool CheckForKeyword(string word);
string GenInternalName(storeType type);

// Function prototypes (stage 1)
void ExecStmts();
void ExecStmt();
void ReadStmt();
void WriteStmt();
void AssignStmt();
void Express();
void Term();
void Factor();
void Terms();
void Expresses();
void Part();
void Factors();
void Code(string oper_ator, string operand1 = "", string operand2 = "");
void EmitReadCode(string operand1);
void EmitWriteCode(string operand1);
void EmitAdditionCode(string operand1, string operand2);
void EmitSubtractionCode(string operand1, string operand2);
void EmitNegationCode(string operand1);
void EmitNotCode(string operand1);
void EmitDivisionCode(string operand1, string operand2);
void EmitModulusCode(string operand1, string operand2);
void EmitMultiplicationCode(string operand1, string operand2);
void EmitAndCode(string operand1, string operand2);
void EmitOrCode(string operand1, string operand2);
void EmitEqualsCode(string operand1, string operand2);
void EmitNotEqualsCode(string operand1, string operand2);
void EmitLessThanOrEqualToCode(string operand1, string operand2);
void EmitGreaterThanOrEqualToCode(string operand2, string operand1);
void EmitLessThanCode(string operand1, string operand2);
void EmitGreaterThanCode(string operand2, string operand1);
void EmitAssignCode(string operand1,string operand2);
bool CheckForRelationalOperator(string oper_ator);
bool CheckForAddLevOperator(string oper_ator);
bool CheckForMultLevOperator(string oper_ator);
bool IsTokenAnInt(string name);
bool IsTokenABool(string name);
bool IsNameInSymbolTable(string name);
bool CheckForTempName(string name);
void FreeTemp();
string GetTemp();
string GetLabel();
void PushOperator(string name);
void PushOperand(string name);
string PopOperator();
string PopOperand();
modes WhichMode(string name);
int FindIndex(string name);
int FindIndexOfTrue();
int FindIndexOfFalse();

// Function prototypes (stage 2)
void IfStmt();
void ElsePt();
void WhileStmt();
void RepeatStmt();
void EmitThenCode(string operand);
void EmitElseCode(string operand);
void EmitPostIfCode(string operand);
void EmitWhileCode();
void EmitDoCode(string operand);
void EmitPostWhileCode(string operand1, string operand2);
void EmitRepeatCode();
void EmitUntilCode(string operand1, string operand2);

int main(int argc, char **argv)
{
	// This program is the stage0 compiler for Pascallite. It will accept
	// input from argv[1], generating a listing to argv[2], and object code to
	// argv[3].
	sourceFile.open(argv[1]);
	listingFile.open(argv[2]);
	objectFile.open(argv[3]);
	
	symbolTable.resize(MAX_SYMBOL_TABLE_SIZE);
	
	for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i += 1)
	{
		symbolTable[i].externalName = "";
	}
	
	CreateListingHeader();
	Parser();
	CreateListingTrailer();
	
	sourceFile.close();
	listingFile.close();
	objectFile.close();

	return 0;
}

void CreateListingHeader()
{
	time_t now = time (NULL);
	
	// Line numbers and source statements should be aligned under the headings.
	listingFile << "STAGE2:  " << "BRIAN LEARY, JOSEPH LYNCH   " << ctime(&now) << '\n';
	listingFile << "LINE NO.              SOURCE STATEMENT\n\n";
}

void Parser()
{
	// Charac must be initialized to the first character of the source file
	NextChar();
	
	// A call to NextToken() has two effects
	// (1) the variable, token, is assigned the value of the next token
	// (2) the next token is read from the source file in order to make
	// the assignment. The value returned by NextToken() is also
	// the next token.
	if (NextToken() != "program")
	{
		Error("Keyword \"program\" expected");
	}
	
	// Parser implements the grammar rules, calling first rule
	Prog();
	
}

// Prints the end of the listing file to the listing file. If the compiler
// makes it here, there weren't any errors.
void CreateListingTrailer()
{
	listingFile << "\nCOMPILATION TERMINATED      0 ERRORS ENCOUNTERED\n";
}

// Token should be "program"
void Prog()
{
	if (token != "program")
	{
		Error("Keyword \"program\" expected");
	}
	ProgStmt();
	if (token == "const") 
		Consts();
	if (token == "var") 
		Vars();
	if (token != "begin")
	{
		Error("keyword \"begin\" expected");
	}
	BeginEndStmt(false);
	if (token[0] != END_OF_FILE)
	{
		Error("no text may follow \"end\" expected");
	}
}

// Token should be "program"
void ProgStmt()
{
	string x;
	if (token != "program")
	{
		Error("keyword \"program\" expected");
	}
	
	x = NextToken();
	if (!CheckNonKeyID(x))
	{
		Error("program name expected");
	}
	
	if (NextToken() != ";")
	{
		Error("semicolon expected");
	}
	
	NextToken();
	
	Insert(x, PROG_NAME, CONSTANT, x, NO, 0);
	
	Code("program");
}

// Token should be "const"
void Consts() 
{
	if (token != "const")
	{
		Error("keyword \"const\" expected");
	}
	if (!CheckNonKeyID(NextToken()))
	{
		Error("non-keyword identifier must follow \"const\"");
	}
	ConstStmts();
}

// Token should be "var"
void Vars() 
{
	if (token != "var")
		Error("keyword \"var\" expected");
	if (!CheckNonKeyID(NextToken()))
		Error("non-keyword identifier must follow \"var\"");
	VarStmts();
}

// Token should be "begin"
void BeginEndStmt(bool internalBeginEnd)
{
	if (token != "begin")
		Error("keyword \"begin\" expected");
	
	NextToken();
	
	ExecStmts();
	
	if (token != "end")
		Error("keyword \"end\" expected");
		
	NextToken();
	
	if (internalBeginEnd)
	{
		if (token == ";")
		{
			Code("end", ";");
		}
		else
		{
			Error("semicolon expected after end");
		}
	}
	else
	{
		if (token == ".")
		{
			Code("end", ".");
			token = NextToken();
		}
		else
		{
			Error("\".\" expected after end");
		}
	}
}

// Token should be read, write, a non-key ID, or end
void ExecStmts()
{
	if (token != "end" && token != "until" && token != "do")
	{
		ExecStmt();
		
		ExecStmts();
	}
}

void ExecStmt()
{
	if (token == "read")
	{
		ReadStmt();
	}
	else if (token == "write")
	{
		WriteStmt();
	}
	else if (CheckNonKeyID(token))
	{
		AssignStmt();
	}
	
	/*
	STAGE 2
	*/
	else if (token == "if")
	{
		IfStmt();
	}
	else if (token == "while")
	{
		WhileStmt();
	}
	else if (token == "repeat")
	{
		RepeatStmt();
	}
	else if (token == ";")
	{
		// NULL_STMT
		NextToken();
	}
	else if (token == "begin")
	{
		BeginEndStmt(true);
	}
	/*
	STAGE 2
	*/
	
	else
	{
		Error("non-keyword identifier, \"read\", \"write\", \"if\", \"while\", \"repeat\", \";\", or \"begin\" expected");
	}
}

// Token should be read
void ReadStmt()
{
	string x;
	if (token != "read")
		Error("\"read\" expected");
	if (NextToken() != "(")
		Error("\'(\' expected");
	
	NextToken();
	
	x = Ids();
	
	if (token != ")")
		Error("\',\' or \')\' expected");
	
	if (NextToken() != ";")
		Error("semicolon expected");
	
	Code("read", x);
}

// Token should be write
void WriteStmt()
{
	string x;
	if (token != "write")
		Error("\"write\" expected");
	if (NextToken() != "(")
		Error("\'(\' expected");
	
	NextToken();
	
	x = Ids();
	
	if (token != ")")
		Error("\',\' or \')\' expected");
	
	if (NextToken() != ";")
		Error("semicolon expected");
	
	Code("write", x);
}

// Token should be a non-key ID
void AssignStmt()
{
	if (!CheckNonKeyID(token))
		Error("non-key ID expected");
	
	PushOperand(token);
	
	if (NextToken() != ":=")
		Error("\":=\" expected");
	
	PushOperator(token);
	
	Express();
	
	if (token != ";")
		Error("semicolon expected");
	
	string operand1 = operandStk.top();
	operandStk.pop();
	string operand2 = operandStk.top();
	operandStk.pop();
	
	Code(PopOperator(), operand1, operand2);
}

// Token should be if
void IfStmt()
{
	if (token != "if")
	{
		Error("\"if\" expected"); 
	}
	
	Express();
	
	if (token != "then")
	{
		Error("\"then\" expected after if statement"); 
	}
	
	Code("then", PopOperand());
	
	NextToken();
	
	ExecStmt();
	
	NextToken();
	
	ElsePt();
}

void ElsePt()
{
	if (token == "else")
	{
		Code("else", PopOperand());
		
		NextToken();
		
		ExecStmt();
	}
	
	Code("post_if", PopOperand());
}

// Token should be while
void WhileStmt()
{
	string operand1, operand2;
	if (token != "while")
	{
		Error("\"while\" expected");
	}
	
	Code("while");
	
	Express();
	
	if (token != "do")
	{
		Error("\"do\" expected after while"); 
	}
	
	operand1 = PopOperand();
	Code("do", operand1);
	
	NextToken();
	
	ExecStmt();
	
	operand1 = PopOperand();
	operand2 = PopOperand();
	
	Code("post_while", operand1, operand2);
}

// Token should be repeat
void RepeatStmt()
{
	if (token != "repeat")
	{
		Error("\"repeat\" expected"); 
	}
	
	Code("repeat");
	
	NextToken();
	
	ExecStmts();
	
	if (token != "until")
	{
		Error("\"until\" expected after repeat"); 
	}
	
	Express();
	
	string operand1 = PopOperand();
	string operand2 = PopOperand();
	
	Code("until", operand1, operand2);
}

void Express()
{
	Term();
	
	Expresses();
}

void Term()
{
	Factor();
	
	Terms();
}

void Factor()
{
	Part();
	
	Factors();
}

void Terms()
{
	if (CheckForAddLevOperator(token))
	{
		PushOperator(token);
		
		Factor();
		
		string operand1 = operandStk.top();
		operandStk.pop();
		string operand2 = operandStk.top();
		operandStk.pop();
		
		Code(PopOperator(), operand1, operand2);
		
		Terms();
	}
}

void Expresses()
{
	if (CheckForRelationalOperator(token))
	{
		PushOperator(token);
		
		Term();
		
		string operand1 = operandStk.top();
		operandStk.pop();
		string operand2 = operandStk.top();
		operandStk.pop();
		
		Code(PopOperator(), operand1, operand2);
		
		Expresses();
	}
}

void Factors()
{
	if (CheckForMultLevOperator(token))
	{
		PushOperator(token);
		
		Part();
		
		string operand1 = operandStk.top();
		operandStk.pop();
		string operand2 = operandStk.top();
		operandStk.pop();
		
		Code(PopOperator(), operand1, operand2);
		
		Factors();
	}
}

void Part()
{
	NextToken();
	
	if (token == "not")
	{
		token = NextToken();
		if (token == "(")
		{
			Express();
			
			if (token != ")")
			{
				Error("\')\' expected");
			}
			
			Code("not", PopOperand());
		}
		else if (token == "false")
		{
			PushOperand("true");
		}
		else if (token == "true")
		{
			PushOperand("false");
		}
		else if (CheckNonKeyID(token))
		{
			Code("not", token);
		}
		else
		{
			Error("illegal symbol follows \"not\"");
		}
		
		NextToken();
	}
	else if (token == "+")
	{
		token = NextToken();
		if (token == "(")
		{
			Express();
			
			if (token != ")")
			{
				Error("\')\' expected");
			}
		}
		else if (IsTokenAnInt(token) || CheckNonKeyID(token))
		{
			PushOperand(token);
		}
		else
		{
			Error("illegal symbol follows \'+\'");
		}
		
		NextToken();
	}
	else if (token == "-")
	{
		token = NextToken();
		if (token == "(")
		{
			Express();
			
			if (token != ")")
			{
				Error("\')\' expected");
			}
			
			Code("neg", PopOperand());
		}
		else if (IsTokenAnInt(token))
		{
			PushOperand("-" + token);
		}
		else if (CheckNonKeyID(token))
		{
			Code("neg", token);
		}
		else
		{
			Error("illegal symbol follows \'-\'");
		}
		
		NextToken();
	}
	else if (token == "(")
	{
		Express();

		if (token != ")")
		{
			Error("\')\' expected");
		}
		
		NextToken();
	}
	else if (IsTokenAnInt(token) || IsTokenABool(token) || CheckNonKeyID(token))
	{
		PushOperand(token);
		NextToken();
	}
}

void Code(string oper_ator, string operand1, string operand2)
{
	if (oper_ator == "program")
	{
		objectFile << "STRT  NOP          " << symbolTable[0].externalName << " - BRIAN LEARY, JOSEPH LYNCH\n";
	}
		
	else if (oper_ator == "end")
	{
		if (operand1 == ".")
		{
			objectFile << "      HLT          \n";
			
			for (unsigned int i = 0; i < symbolTable.size(); i += 1)
			{
				if (symbolTable[i].externalName == "")
				{
					break;
				}
				else
				{
					if (symbolTable[i].alloc == YES)
					{
						if (symbolTable[i].mode == CONSTANT)
						{
							objectFile << setw(4) << left << symbolTable[i].internalName;
							objectFile << "  DEC ";
							
							if (symbolTable[i].value[0] == '-')
							{
								objectFile << "-";
								string tempString = "";
								for (unsigned int j = 1; j < symbolTable[i].value.length(); j += 1)
									tempString += symbolTable[i].value[j];
								
								objectFile << setfill('0') << right << setw(3) << tempString;
							}
							else
							{
								objectFile << setfill('0') << right << setw(4) << symbolTable[i].value;
							}
							
							objectFile << setfill(' ') << "     ";
							objectFile << symbolTable[i].externalName << "\n";
							
						}
						else
						{
							objectFile << setw(4) << left << symbolTable[i].internalName;
							objectFile << "  BSS 0001";
							objectFile << setfill(' ') << "     ";
							objectFile << symbolTable[i].externalName << "\n";
						}
					}
				}
			}
			
			objectFile << "      END STRT     \n";
		
		}
		else if (operand1 == ";")
		{
			// do nothing
		}
		else
		{
			Error("illegal character follows end");
		}
	}
		
	else if (oper_ator == "read")
	{
		EmitReadCode(operand1);
	}
		
	else if (oper_ator == "write")
	{
		EmitWriteCode(operand1);
	}
		
	// This must be binary '+'
	else if (oper_ator == "+")
	{
		EmitAdditionCode(operand1, operand2);
	}
		
	// This must be binary '-'
	else if (oper_ator == "-")
	{
		EmitSubtractionCode(operand1, operand2);
	}
		
	// This must be unary '-'
	else if (oper_ator == "neg")
	{
		EmitNegationCode(operand1);
	}
		
	else if (oper_ator == "not")
	{
		EmitNotCode(operand1);
	}
	
	else if (oper_ator == "*")
	{
		EmitMultiplicationCode(operand1, operand2);
	}
	
	else if (oper_ator == "div")
	{
		EmitDivisionCode(operand1, operand2);
	}
	
	else if (oper_ator == "mod")
	{
		EmitModulusCode(operand1, operand2);
	}
	
	else if (oper_ator == "and")
	{
		if (operand1 == "true" || operand1 == "false")
		{
			if (operand1 == "true")
			{
				if (FindIndexOfTrue() != -1)
				{
					operand1 = symbolTable[FindIndexOfTrue()].externalName;
				}
				else
				{
					Insert("LOWERCASETRUE", BOOLEAN, CONSTANT, "1", YES, 1);
					operand1 = "true";
				}
			}
			else
			{
				if (FindIndexOfFalse() != -1)
				{
					operand1 = symbolTable[FindIndexOfFalse()].externalName;
				}
				else
				{
					Insert("LOWERCASEFALSE", BOOLEAN, CONSTANT, "0", YES, 1);
					operand1 = "false";
				}
			}
		}
		
		if (operand2 == "true" || operand2 == "false")
		{
			if (operand2 == "true")
			{
				if (FindIndexOfTrue() != -1)
				{
					operand2 = symbolTable[FindIndexOfTrue()].externalName;
				}
				else
				{
					Insert("LOWERCASETRUE", BOOLEAN, CONSTANT, "1", YES, 1);
					operand2 = "true";
				}
			}
			else
			{
				if (FindIndexOfFalse() != -1)
				{
					operand2 = symbolTable[FindIndexOfFalse()].externalName;
				}
				else
				{
					Insert("LOWERCASEFALSE", BOOLEAN, CONSTANT, "0", YES, 1);
					operand2 = "false";
				}
			}
		}
		
		EmitAndCode(operand1, operand2);
	}
	
	else if (oper_ator == "or")
	{
		if (operand1 == "true" || operand1 == "false")
		{
			if (operand1 == "true")
			{
				if (FindIndexOfTrue() != -1)
				{
					operand1 = symbolTable[FindIndexOfTrue()].externalName;
				}
				else
				{
					Insert("LOWERCASETRUE", BOOLEAN, CONSTANT, "1", YES, 1);
					operand1 = "true";
				}
			}
			else
			{
				if (FindIndexOfFalse() != -1)
				{
					operand1 = symbolTable[FindIndexOfFalse()].externalName;
				}
				else
				{
					Insert("LOWERCASEFALSE", BOOLEAN, CONSTANT, "0", YES, 1);
					operand1 = "false";
				}
			}
		}
		
		if (operand2 == "true" || operand2 == "false")
		{
			if (operand2 == "true")
			{
				if (FindIndexOfTrue() != -1)
				{
					operand2 = symbolTable[FindIndexOfTrue()].externalName;
				}
				else
				{
					Insert("LOWERCASETRUE", BOOLEAN, CONSTANT, "1", YES, 1);
					operand2 = "true";
				}
			}
			else
			{
				if (FindIndexOfFalse() != -1)
				{
					operand2 = symbolTable[FindIndexOfFalse()].externalName;
				}
				else
				{
					Insert("LOWERCASEFALSE", BOOLEAN, CONSTANT, "0", YES, 1);
					operand2 = "false";
				}
			}
		}
		
		EmitOrCode(operand1, operand2);
	}
	
	else if (oper_ator == "=")
	{
		EmitEqualsCode(operand1, operand2);
	}
	
	else if (oper_ator == "<>")
	{
		EmitNotEqualsCode(operand1, operand2);
	}
	
	else if (oper_ator == "<=")
	{
		EmitLessThanOrEqualToCode(operand1, operand2);
	}
	
	else if (oper_ator == ">=")
	{
		EmitGreaterThanOrEqualToCode(operand1, operand2);
	}
	
	else if (oper_ator == "<")
	{
		EmitLessThanCode(operand1, operand2);
	}
	
	else if (oper_ator == ">")
	{
		EmitGreaterThanCode(operand1, operand2);
	}
	
	else if (oper_ator == ":=")
	{
		if (operand1 == "true" || operand1 == "false")
		{
			if (operand1 == "true")
			{
				if (FindIndexOfTrue() != -1)
				{
					operand1 = symbolTable[FindIndexOfTrue()].externalName;
				}
				else
				{
					Insert("LOWERCASETRUE", BOOLEAN, CONSTANT, "1", YES, 1);
					operand1 = "true";
				}
			}
			else
			{
				if (FindIndexOfFalse() != -1)
				{
					operand1 = symbolTable[FindIndexOfFalse()].externalName;
				}
				else
				{
					Insert("LOWERCASEFALSE", BOOLEAN, CONSTANT, "0", YES, 1);
					operand1 = "false";
				}
			}
		}
		
		EmitAssignCode(operand1, operand2);
	}
	
	/*
	STAGE 2
	*/
	
	else if (oper_ator == "then")
	{
		if (operand1 == "true" || operand1 == "false")
		{
			if (operand1 == "true")
			{
				if (FindIndexOfTrue() != -1)
				{
					operand1 = symbolTable[FindIndexOfTrue()].externalName;
				}
				else
				{
					Insert("LOWERCASETRUE", BOOLEAN, CONSTANT, "1", YES, 1);
					operand1 = "true";
				}
			}
			else
			{
				if (FindIndexOfFalse() != -1)
				{
					operand1 = symbolTable[FindIndexOfFalse()].externalName;
				}
				else
				{
					Insert("LOWERCASEFALSE", BOOLEAN, CONSTANT, "0", YES, 1);
					operand1 = "false";
				}
			}
		}
		
		EmitThenCode(operand1);
	}
	else if (oper_ator == "else")
	{
		EmitElseCode(operand1);
	}
	else if (oper_ator == "post_if")
	{
		EmitPostIfCode(operand1);
	}
	else if (oper_ator == "while")
	{
		EmitWhileCode();
	}
	else if (oper_ator == "do")
	{
		if (operand1 == "true" || operand1 == "false")
		{
			if (operand1 == "true")
			{
				if (FindIndexOfTrue() != -1)
				{
					operand1 = symbolTable[FindIndexOfTrue()].externalName;
				}
				else
				{
					Insert("LOWERCASETRUE", BOOLEAN, CONSTANT, "1", YES, 1);
					operand1 = "true";
				}
			}
			else
			{
				if (FindIndexOfFalse() != -1)
				{
					operand1 = symbolTable[FindIndexOfFalse()].externalName;
				}
				else
				{
					Insert("LOWERCASEFALSE", BOOLEAN, CONSTANT, "0", YES, 1);
					operand1 = "false";
				}
			}
		}
		
		EmitDoCode(operand1);
	}
	else if (oper_ator == "post_while")
	{
		EmitPostWhileCode(operand1, operand2);
	}
	else if (oper_ator == "repeat")
	{
		EmitRepeatCode();
	}
	else if (oper_ator == "until")
	{
		if (operand1 == "true" || operand1 == "false")
		{
			if (operand1 == "true")
			{
				if (FindIndexOfTrue() != -1)
				{
					operand1 = symbolTable[FindIndexOfTrue()].externalName;
				}
				else
				{
					Insert("LOWERCASETRUE", BOOLEAN, CONSTANT, "1", YES, 1);
					operand1 = "true";
				}
			}
			else
			{
				if (FindIndexOfFalse() != -1)
				{
					operand1 = symbolTable[FindIndexOfFalse()].externalName;
				}
				else
				{
					Insert("LOWERCASEFALSE", BOOLEAN, CONSTANT, "0", YES, 1);
					operand1 = "false";
				}
			}
		}
		
		EmitUntilCode(operand1, operand2);
	}
	
	/*
	STAGE2
	*/
	else
	{		
		Error("undefined operation");
	}
}

// Read in value 
void EmitReadCode(string operand1)
{
	string currentName = "";
	
	int index;
	
	operand1 += " ";
	
	for (unsigned int j = 0; j < operand1.length(); j += 1)
	{
		if (operand1[j] == ',' || operand1[j] == ' ')
		{
			index = FindIndex(currentName);
			
			if (WhichMode(currentName) != VARIABLE)
				Error("can't change constant's value");
			else
				objectFile << "      RDI " << setw(9) << left << symbolTable[index].internalName << "read(" << symbolTable[index].externalName << ")\n";
			
			currentName = "";
		}
		else
		{
			currentName = currentName + operand1[j];
		}
	}
}

// Print value
void EmitWriteCode(string operand1)
{
	string currentName = "";
	
	int index;
	
	operand1 += " ";
	
	for (unsigned int j = 0; j < operand1.length(); j += 1)
	{
		if (operand1[j] == ',' || operand1[j] == ' ')
		{
			index = FindIndex(currentName);
			
			objectFile << "      PRI " << symbolTable[index].internalName << "       write(" << symbolTable[index].externalName << ")\n";
			
			currentName = "";
		}
		else
		{
			currentName = currentName + operand1[j];
		}
	}
}

// Add operand1 to operand2.
void EmitAdditionCode(string operand1, string operand2) 
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName;
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
	if (currentARegister[0] == 'T' && currentARegister != operand1 && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	if (currentARegister == operand1)
	{
		objectFile << "      IAD " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
		           << symbolTable[indexOfOperand2].externalName << " + " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else if (currentARegister == operand2)
	{
		objectFile << "      IAD " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " + " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
		objectFile << "      IAD " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " + " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = INTEGER;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Subtract operand2 from operand1.
void EmitSubtractionCode(string operand1, string operand2) 
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName;
	
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
		
		
	if (currentARegister[0] == 'T' && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	
	
	if (currentARegister != operand2)
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
	}
	objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
			   << symbolTable[indexOfOperand2].externalName << " - " << symbolTable[indexOfOperand1].externalName << "\n";
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = INTEGER;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Negate operand1.
void EmitNegationCode(string operand1)
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfTemp;
	string tempName;
	
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER)
		Error("illegal type");
		
	if (currentARegister[0] == 'T')
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1)
	{
		currentARegister = "";
	}
	
	objectFile << "      LDA " << setw(9) << left << "ZERO" << "\n";
	
	if (!IsNameInSymbolTable("ZERO"))
	{
		Insert("ZERO", INTEGER, CONSTANT, "0", YES, 1);
	}
	
	objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
			   << "-" << symbolTable[indexOfOperand1].externalName << "\n";
	
	if (operand1[0] == 'T')
		FreeTemp();
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = INTEGER;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Divide operand2 by operand1.
void EmitDivisionCode(string operand1, string operand2) 
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName;
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
		
		
	if (currentARegister[0] == 'T' && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	
	
	if (currentARegister == operand2)
	{
		objectFile << "      IDV " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " div " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
		objectFile << "      IDV " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " div " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	
	
	
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = INTEGER;
	currentARegister = tempName;
	operandStk.push(tempName);
}

void EmitModulusCode(string operand1, string operand2)
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName;
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
		
		
	if (currentARegister[0] == 'T' && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	
	
	if (currentARegister != operand2)
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
	}
	
	objectFile << "      IDV " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		       << symbolTable[indexOfOperand2].externalName << " mod " << symbolTable[indexOfOperand1].externalName << "\n";
	
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = INTEGER;
	symbolTable[indexOfTemp].alloc = YES;
	objectFile << "      STQ " << setw(9) << left << symbolTable[indexOfTemp].internalName 
			   << "store remainder in memory\n";
    objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfTemp].internalName 
			   << "load remainder from memory\n";
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Multiply operand2 by operand1
void EmitMultiplicationCode(string operand1, string operand2) 
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName;
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
		
		
		
	if (currentARegister[0] == 'T' && currentARegister != operand1 && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	
	
	if (currentARegister == operand1)
	{
		objectFile << "      IMU " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
		           << symbolTable[indexOfOperand2].externalName << " * " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else if (currentARegister == operand2)
	{
		objectFile << "      IMU " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " * " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
		objectFile << "      IMU " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " * " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	
	
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = INTEGER;
	currentARegister = tempName;
	operandStk.push(tempName);
	
}

// "not" operand1 to operand2.
void EmitNotCode(string operand1)
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfTemp;
	string tempName, labelName;
	
	if (symbolTable[indexOfOperand1].dataType != BOOLEAN)
		Error("illegal type");
		
	if (currentARegister[0] == 'T' && currentARegister != operand1)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1)
	{
		currentARegister = "";
	}
	
	if (currentARegister != operand1)
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand1].internalName << "\n";
	}
	
	labelName = GetLabel();
	
	objectFile << "      AZJ " << setw(9) << left << labelName << "not " << symbolTable[indexOfOperand1].externalName << "\n";
	
	objectFile << "      LDA " << setw(9) << left << "FALS     \n";
	
	if (!IsNameInSymbolTable("FALSE"))
	{
		Insert("FALSE", BOOLEAN, CONSTANT, "0", YES, 1);
	}
	
	objectFile << "      UNJ " << setw(4) << left << labelName << "+1" << "   \n";
	
	objectFile << setw(6) << left << labelName << "LDA " << setw(9) << left << "TRUE     \n";
	
	if (!IsNameInSymbolTable("TRUE"))
	{
		Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
	}	
	
	if (operand1[0] == 'T')
		FreeTemp();
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// "and" operand1 to operand2.
void EmitAndCode(string operand1, string operand2) 
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName;
	
	if (symbolTable[indexOfOperand1].dataType != BOOLEAN ||
		symbolTable[indexOfOperand2].dataType != BOOLEAN)
		Error("operator and requires boolean operands");
		
	if (currentARegister[0] == 'T' && currentARegister != operand1 && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	
	if (currentARegister == operand1)
	{
		objectFile << "      IMU " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
		           << symbolTable[indexOfOperand2].externalName << " and " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else if (currentARegister == operand2)
	{
		objectFile << "      IMU " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " and " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
		objectFile << "      IMU " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " and " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// "or" operand1 to operand2.
void EmitOrCode(string operand1, string operand2) 
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName, labelName;
	
	if (symbolTable[indexOfOperand1].dataType != BOOLEAN ||
		symbolTable[indexOfOperand2].dataType != BOOLEAN)
		Error("operator or requires boolean operands");
		
	if (currentARegister[0] == 'T' && currentARegister != operand1 && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	if (currentARegister == operand1)
	{
		objectFile << "      IAD " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
		           << symbolTable[indexOfOperand2].externalName << " or " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else if (currentARegister == operand2)
	{
		objectFile << "      IAD " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " or " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
		objectFile << "      IAD " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " or " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	
	labelName = GetLabel();
	
	objectFile << "      AZJ " << setw(4) << left << labelName << "+1" << "   \n";
	
	objectFile << setw(6) << left << labelName << "LDA " << setw(9) << left << "TRUE     \n";
	
	if (!IsNameInSymbolTable("TRUE"))
	{
		Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
	}
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Test whether operand2 equals operand1.
void EmitEqualsCode(string operand1, string operand2) 
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName, labelName;
	
	if (symbolTable[indexOfOperand1].dataType != symbolTable[indexOfOperand2].dataType)
		Error("incompatible types");
		
	if (currentARegister[0] == 'T' && currentARegister != operand1 && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	if (currentARegister == operand1)
	{
		objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
		           << symbolTable[indexOfOperand2].externalName << " = " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else if (currentARegister == operand2)
	{
		objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " = " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
		objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " = " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	
	labelName = GetLabel();
	
	objectFile << "      AZJ " << setw(4) << left << labelName << "     \n";
	
	objectFile << "      LDA " << setw(9) << left << "FALS     \n";
	
	if (!IsNameInSymbolTable("FALSE"))
	{
		Insert("FALSE", BOOLEAN, CONSTANT, "0", YES, 1);
	}
	
	objectFile << "      UNJ " << setw(4) << left << labelName << "+1" << "   \n";
	
	objectFile << setw(6) << left << labelName << "LDA " << setw(9) << left << "TRUE     \n";
	
	if (!IsNameInSymbolTable("TRUE"))
	{
		Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
	}
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Test whether operand2 doesn't equal operand1.
void EmitNotEqualsCode(string operand1, string operand2)
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName, labelName;
	
	if (symbolTable[indexOfOperand1].dataType != symbolTable[indexOfOperand2].dataType)
		Error("incompatible types");
		
	if (currentARegister[0] == 'T' && currentARegister != operand1 && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	if (currentARegister == operand1)
	{
		objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
		           << symbolTable[indexOfOperand2].externalName << " <> " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else if (currentARegister == operand2)
	{
		objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " <> " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	else
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
		objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
		           << symbolTable[indexOfOperand2].externalName << " <> " << symbolTable[indexOfOperand1].externalName << "\n";
	}
	
	labelName = GetLabel();
	
	objectFile << "      AZJ " << setw(4) << left << labelName << "+1" << "   \n";
	
	objectFile << setw(6) << left << labelName << "LDA " << setw(9) << left << "TRUE     \n";
	
	if (!IsNameInSymbolTable("TRUE"))
	{
		Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
	}
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Test whether operand2 is less than or equal to operand1.
void EmitLessThanOrEqualToCode(string operand1, string operand2)
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName, labelName;
	
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
		
		
	if (currentARegister[0] == 'T' && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	
	if (currentARegister != operand2)
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
	}
	objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
			   << symbolTable[indexOfOperand2].externalName << " <= " << symbolTable[indexOfOperand1].externalName << "\n";
	
	labelName = GetLabel();
	
	objectFile << "      AMJ " << setw(4) << left << labelName << "     \n";
	objectFile << "      AZJ " << setw(4) << left << labelName << "     \n";
	
	objectFile << "      LDA " << setw(9) << left << "FALS     \n";
	
	if (!IsNameInSymbolTable("FALSE"))
	{
		Insert("FALSE", BOOLEAN, CONSTANT, "0", YES, 1);
	}
	
	objectFile << "      UNJ " << setw(4) << left << labelName << "+1" << "   \n";
	
	objectFile << setw(6) << left << labelName << "LDA " << setw(9) << left << "TRUE     \n";
	
	if (!IsNameInSymbolTable("TRUE"))
	{
		Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
	}
	
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Test whether operand2 is greater than or equal to operand1.
void EmitGreaterThanOrEqualToCode(string operand2, string operand1)
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName, labelName;
	
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
		
		
	if (currentARegister[0] == 'T' && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	
	if (currentARegister != operand1)
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand1].internalName << "\n";
	}
	objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
			   << symbolTable[indexOfOperand1].externalName << " >= " << symbolTable[indexOfOperand2].externalName << "\n";
	
	
	
	labelName = GetLabel();
	
	objectFile << "      AMJ " << setw(4) << left << labelName << "     \n";
	
	objectFile << "      LDA " << setw(9) << left << "TRUE     \n";
	
	if (!IsNameInSymbolTable("TRUE"))
	{
		Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
	}
	
	objectFile << "      UNJ " << setw(4) << left << labelName << "+1" << "   \n";
	
	objectFile << setw(6) << left << labelName << "LDA " << setw(9) << left << "FALS     \n";
	
	if (!IsNameInSymbolTable("FALSE"))
	{
		Insert("FALSE", BOOLEAN, CONSTANT, "0", YES, 1);
	}
	
	
	
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Test whether operand2 is less than operand1.
void EmitLessThanCode(string operand1, string operand2)
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName, labelName;
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
		
		
	if (currentARegister[0] == 'T' && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	
	if (currentARegister != operand2)
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand2].internalName << "\n";
	}
	objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand1].internalName 
			   << symbolTable[indexOfOperand2].externalName << " < " << symbolTable[indexOfOperand1].externalName << "\n";
	
	labelName = GetLabel();
	
	objectFile << "      AMJ " << setw(4) << left << labelName << "     \n";
	
	objectFile << "      LDA " << setw(9) << left << "FALS     \n";
	
	if (!IsNameInSymbolTable("FALSE"))
	{
		Insert("FALSE", BOOLEAN, CONSTANT, "0", YES, 1);
	}
	
	objectFile << "      UNJ " << setw(4) << left << labelName << "+1" << "   \n";
	
	objectFile << setw(6) << left << labelName << "LDA " << setw(9) << left << "TRUE     \n";
	
	if (!IsNameInSymbolTable("TRUE"))
	{
		Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
	}
	
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Test whether operand2 is greater than operand1.
void EmitGreaterThanCode(string operand2, string operand1)
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	int indexOfTemp;
	string tempName, labelName;
	
	
	if (symbolTable[indexOfOperand1].dataType != INTEGER ||
		symbolTable[indexOfOperand2].dataType != INTEGER)
		Error("illegal type");
		
		
	if (currentARegister[0] == 'T' && currentARegister != operand2)
	{
		objectFile << "      STA " << setw(9) << left << currentARegister << "deassign AReg\n";
		indexOfTemp = FindIndex(currentARegister);
		symbolTable[indexOfTemp].alloc = YES;
		
		currentARegister = "";
	}
	else if (currentARegister != operand1 && currentARegister != operand2)
	{
		currentARegister = "";
	}
	
	if (currentARegister != operand1)
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand1].internalName << "\n";
	}
	objectFile << "      ISB " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
			   << symbolTable[indexOfOperand1].externalName << " > " << symbolTable[indexOfOperand2].externalName << "\n";
	
	labelName = GetLabel();
	
	objectFile << "      AMJ " << setw(4) << left << labelName << "     \n";
	objectFile << "      AZJ " << setw(4) << left << labelName << "     \n";
	
	objectFile << "      LDA " << setw(9) << left << "TRUE     \n";
	
	if (!IsNameInSymbolTable("TRUE"))
	{
		Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
	}
	
	objectFile << "      UNJ " << setw(4) << left << labelName << "+1" << "   \n";
	
	objectFile << setw(6) << left << labelName << "LDA " << setw(9) << left << "FALS     \n";
	
	if (!IsNameInSymbolTable("FALSE"))
	{
		Insert("FALSE", BOOLEAN, CONSTANT, "0", YES, 1);
	}
	
	if (operand1[0] == 'T')
		FreeTemp();
	if (operand2[0] == 'T')
		FreeTemp();
	tempName = GetTemp();
	indexOfTemp = FindIndex(tempName);
	symbolTable[indexOfTemp].dataType = BOOLEAN;
	currentARegister = tempName;
	operandStk.push(tempName);
}

// Assign the value of operand1 to operand2.
void EmitAssignCode(string operand1, string operand2) 
{
	int indexOfOperand1 = FindIndex(operand1);
	int indexOfOperand2 = FindIndex(operand2);
	
	if (symbolTable[indexOfOperand1].dataType != symbolTable[indexOfOperand2].dataType)
		Error("incompatible types");
	if (symbolTable[indexOfOperand2].mode != VARIABLE)
		Error("symbol on left-hand side of assignment must have a storage mode of VARIABLE");
	if (symbolTable[indexOfOperand1].externalName == symbolTable[indexOfOperand2].externalName)
	{
		return;
	}
	
	
	if (symbolTable[indexOfOperand1].externalName != currentARegister)
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand1].internalName << "\n";
	}
	objectFile << "      STA " << setw(9) << left << symbolTable[indexOfOperand2].internalName 
			   << symbolTable[indexOfOperand2].externalName << " := " << symbolTable[indexOfOperand1].externalName << "\n";
	
			   
	currentARegister = operand2;
	
	if (symbolTable[indexOfOperand1].internalName[0] == 'T' && symbolTable[indexOfOperand1].internalName != "TRUE")
	{
		FreeTemp();
	}
}

// Emit code that follows "then" and statement predicate.
void EmitThenCode(string operand)
{
	string tempLabel;
	int indexOfOperand = FindIndex(operand);
	
	tempLabel = GetLabel();
	
	if (symbolTable[indexOfOperand].dataType != BOOLEAN)
	{
		Error("predicate must be boolean valued");
	}

	if ((operand[0] != 'T' && CheckNonKeyID(operand)) || operand == "true" || operand == "false")
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand].internalName << "\n";
	}
	objectFile << "      AZJ " << setw(4) << left << tempLabel << "     if false jump to " << tempLabel << "\n";
	
	PushOperand(tempLabel);
	
	if (operand[0] == 'T')
		FreeTemp();
		
	currentARegister = "";
}

// Emit code that follows else clause of if statement
void EmitElseCode(string operand)
{
	string tempLabel;
	
	tempLabel = GetLabel();
	
	objectFile << "      UNJ " << setw(4) << left << tempLabel << "     jump to end if\n";
	
	objectFile << setw(6) << left << operand << "NOP " << setw(9) << left << "         else\n";
	
	PushOperand(tempLabel);
	
	currentARegister = "";
}

// Emit code that follows end of if statement
void EmitPostIfCode(string operand)
{
	objectFile << setw(6) << left << operand << "NOP " << setw(9) << left << "         end if\n";
	
	currentARegister = "";
}

// Emit code that follows while
void EmitWhileCode()
{
	string tempLabel;
	
	tempLabel = GetLabel();
	
	objectFile << setw(6) << left << tempLabel << "NOP " << setw(9) << left << "         while\n";
	
	PushOperand(tempLabel);
	
	currentARegister = "";
}

// Emit code that follows do
void EmitDoCode(string operand)
{
	string tempLabel;
	
	int indexOfOperand = FindIndex(operand);
	
	tempLabel = GetLabel();
	
	if (symbolTable[indexOfOperand].dataType != BOOLEAN)
	{
		Error("predicate must be boolean valued");
	}

	if ((operand[0] != 'T' && CheckNonKeyID(operand)) || operand == "true" || operand == "false")
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand].internalName << "\n";
	}
	objectFile << "      AZJ " << setw(4) << left << tempLabel << "     do\n";
	
	PushOperand(tempLabel);
	
	if (operand[0] == 'T')
		FreeTemp();
		
	currentARegister = "";
}

// Emit code at end of while loop. operand2 is the label of the beginning of the loop,
// operand1 is the label which should follow the end of the loop.
void EmitPostWhileCode(string operand1, string operand2)
{
	objectFile << "      UNJ " << setw(4) << left << operand2 << "     end while\n";
	objectFile << setw(6) << left << operand1 << "NOP " << setw(9) << left << "         \n";
	
	currentARegister = "";
}

// Emit code that follows repeat
void EmitRepeatCode()
{
	string tempLabel;
	
	tempLabel = GetLabel();
	
	objectFile << setw(6) << left << tempLabel << "NOP " << setw(9) << left << "         repeat\n";
	
	PushOperand(tempLabel);
	
	currentARegister = "";
}

// Emit code that follows until and the predicate of loop. operand1 is the value of the
// predicate. operand2 is the label that points to the beginning of the loop
void EmitUntilCode(string operand1, string operand2)
{
	int indexOfOperand1 = FindIndex(operand1);
	
	if (symbolTable[indexOfOperand1].dataType != BOOLEAN)
	{
		Error("predicate must be boolean valued");
	}
	
	if ((operand1[0] != 'T' && CheckNonKeyID(operand1)) || operand1 == "true" || operand1 == "false")
	{
		objectFile << "      LDA " << setw(9) << left << symbolTable[indexOfOperand1].internalName << "\n";
	}
	objectFile << "      AZJ " << setw(4) << left << operand2 << "     until\n";
	
	if (operand1[0] == 'T')
		FreeTemp();
		
	currentARegister = "";
}

void FreeTemp()
{
	currentTempNo--;
	if (currentTempNo < -1)
		Error("compiler error, currentTempNo should be >= –1");
}

string GetTemp()
{
	string temp;
	currentTempNo++;
	temp = "T" + to_string(currentTempNo);
	if (currentTempNo > maxTempNo)
	{
		Insert(temp, UNKNOWN, VARIABLE, "", NO, 1);
		maxTempNo++;
	}
	return temp;
}

string GetLabel()
{
	string temp;
	currentLabelNo++;
	temp = "L" + to_string(currentLabelNo);
	
	return temp;
}

// Push name onto operatorStk
void PushOperator(string name) 
{
	operatorStk.push(name);
}

// Push name onto operandStk
// If name is a literal, also create a symbol table entry for it
void PushOperand(string name) 
{
	if ((IsTokenABool(name) || IsTokenAnInt(name)) && !IsNameInSymbolTable(name))
	{
		if (name == "true")
		{
			if (FindIndexOfTrue() == -1)
			{
				Insert(name, WhichType(name), CONSTANT, name, YES, 1);
			}
		}
		else if (name == "false")
		{
			if (FindIndexOfFalse() == -1)
			{
				Insert(name, WhichType(name), CONSTANT, name, YES, 1);
			}
		}
		else
		{
			Insert(name, WhichType(name), CONSTANT, name, YES, 1);
		}
	}
	
	operandStk.push(name);
}

// Pop name from operatorStk
string PopOperator() 
{
	string returnString;
	if (!operatorStk.empty())
	{
		returnString = operatorStk.top();
		operatorStk.pop();
	}
	else 
		Error("operator stack underflow");
	
	return returnString;
}

// Pop name from operandStk
string PopOperand() 
{
	string returnString;
	if (!operandStk.empty())
	{
		returnString = operandStk.top();
		operandStk.pop();
	}
	else 
		Error("operand stack underflow");
	
	return returnString;
}

// Token should be NON_KEY_ID
void ConstStmts() 
{
	string x, y;
	int indexOfOtherOperand;
	
	if (!CheckNonKeyID(token))
	{
		Error("non-keyword identifier expected");
	}
	x = token;
	
	if (NextToken() != "=")
	{
		Error("\"=\" expected");
	}
	
	y = NextToken();
	if (y != "+"          &&
		y != "-"          &&
		y != "not"        &&
		!CheckNonKeyID(y) &&
		y != "true"       &&
		y != "false"      &&
		WhichType(y) != INTEGER)
		Error("token to right of \"=\" illegal");
	
	if (y == "+" || y == "-")
	{
		if(WhichType(NextToken()) != INTEGER)
			Error("integer expected after sign");
		y = y + token;
	}
	if (y == "not")
	{
		if (WhichType(NextToken()) != BOOLEAN)
			Error("boolean expected after not");
		if (token == "true")
			y = "false";
		else if (token == "false")
			y = "true";
		// Stage 1 allows non-key ID to follow "not" in constant declaration.
		else
		{
			indexOfOtherOperand = FindIndex(token);
			if (symbolTable[indexOfOtherOperand].value == "0")
				y = "true";
			else
				y = "false";
		}
	}
	
	if (NextToken() != ";")
		Error("semicolon expected");
		
	Insert(x, WhichType(y), CONSTANT, WhichValue(y), YES, 1);
	
	token = NextToken();
	if (token != "begin" && token != "var" && !CheckNonKeyID(token))
		Error("non-keyword identifier,\"begin\", or \"var\" expected");
		
	if (CheckNonKeyID(token))
		ConstStmts();
}

// Token should be NON_KEY_ID
void VarStmts() 
{
	string x, y, externalName = "";
	storeType type;
	
	if (!CheckNonKeyID(token))
		Error("non-keyword identifier expected");
	
	x = Ids();
	
	if (token != ":")
		Error("\":\" expected");
		
	token = NextToken();
	if (token != "integer" && token != "boolean")
		Error("illegal type follows \":\"");
	
	if (token == "integer")
	{
		type = INTEGER;
	}
	else
	{
		type = BOOLEAN;
	}
		
	for (unsigned int i = 0; i < token.length(); i += 1)
		token[i] = toupper(token[i]);
	y = token;
	
	if (NextToken() != ";")
		Error("semicolon expected");
	
	Insert(x, type, VARIABLE, "", YES, 1);
	
	token = NextToken();
	if (token != "begin" && !CheckNonKeyID(token))
		Error("non-keyword identifier or \"begin\" expected");
		
	if (CheckNonKeyID(token))
		VarStmts();
}

// Token should be NON_KEY_ID
string Ids() 
{
	string temp, tempString = "";
	if (!CheckNonKeyID(token))
		Error("non-keyword identifier expected");
		
	tempString = token;
	temp = token;
	
	if(NextToken() == ",")
	{
		token = NextToken();
		if (!CheckNonKeyID(token))
			Error("non-keyword identifier expected");
		tempString = temp + "," + Ids();
	}
	
	return tempString;
}

// Create symbol table entry for each identifier in list of external names.
// Multiply inserted names are illegal.
void Insert(string externalName, storeType inType, modes inMode, string inValue, allocation inAlloc, int inUnits)
{
	unsigned int i = 0, currentLength = 0;
	string currentName = "";
	
	externalName += " ";
	
	if (inValue == "true")
		inValue = "1";
	else if (inValue == "false")
		inValue = "0";
	if (inValue.length() > 15)
		inValue = inValue.substr(0, 15);
	
	for (unsigned int j = 0; j < externalName.length(); j += 1)
	{
		if (externalName[j] == ',' || externalName[j] == ' ' || currentLength == 15)
		{
			// Check for multiple name definition
			for (i = 0; i < MAX_SYMBOL_TABLE_SIZE; i += 1)
			{
				if (symbolTable[i].externalName == "")
				{
					break;
				}
				else
				{
					if (symbolTable[i].externalName == currentName)
					{
						Error("multiple name definition");
					}
				}
			}
			
			if (i == MAX_SYMBOL_TABLE_SIZE)
				Error("symbol table overflow");
			
			if (!CheckNonKeyID(currentName)    && 
				!CheckForTempName(currentName) && 
				!IsTokenAnInt(currentName)     && 
				!IsTokenABool(currentName)     &&
				currentName != "ZERO"		   &&
				currentName != "TRUE"		   &&
				currentName != "FALSE"		   &&
				currentName != "LOWERCASETRUE" &&
				currentName != "LOWERCASEFALSE")
				Error("illegal use of keyword");
				
			if (currentName == "TRUE")
				symbolTable[i].internalName = "TRUE";
			else if (currentName == "FALSE")
				symbolTable[i].internalName = "FALS";
			else if (currentName == "ZERO")
				symbolTable[i].internalName = "ZERO";
			else if (isupper(currentName[0]))
				symbolTable[i].internalName = currentName;
			else
				symbolTable[i].internalName = GenInternalName(inType);
			
			if (currentName == "LOWERCASETRUE")
				symbolTable[i].externalName = "true";
			else if (currentName == "LOWERCASEFALSE")
				symbolTable[i].externalName = "false";
			else
				symbolTable[i].externalName = currentName;
			
			symbolTable[i].dataType = inType;
			symbolTable[i].mode = inMode;
			symbolTable[i].value = inValue;
			symbolTable[i].alloc = inAlloc;
			symbolTable[i].units = inUnits;
			
			if (currentLength == 15)
			{
				while (externalName[j] != ',' && externalName[j] != ' ')
				{
					j += 1;
				}
			}
			
			currentName = "";
			currentLength = 0;
		}
		else
		{
			currentName = currentName + externalName[j];
			currentLength += 1;
		}
	}
}

// Tells which data type a name has.
storeType WhichType(string name)
{
	storeType data_type;
	
	bool nameIsAnInt = true, constantAlreadyExists = false;
	
	for (unsigned int i = 0; i < name.length(); i += 1)
	{
		if (!isdigit(name[i]) && name[i] != '+' && name[i] != '-')
			nameIsAnInt = false;
	}
	
	if (nameIsAnInt || name == "true" || name == "false")
	{
		if (name == "true" || name == "false")
			data_type = BOOLEAN;
		else 
			data_type = INTEGER;
	}
	
	// Name is an identifier and hopefully a constant
	else 
	{
		unsigned int i = 0;
		
		for (i = 0; i < symbolTable.size(); i += 1)
		{
			if (symbolTable[i].externalName == "")
				break;
			else if (symbolTable[i].externalName == name && symbolTable[i].mode == CONSTANT)
			{
				constantAlreadyExists = true;
				break;
			}
		}
		
		if (constantAlreadyExists)
			data_type = symbolTable[i].dataType;
		else 
			Error("reference to undefined constant");
	}
		
	return data_type;
}

// Tells which value a name has.
string WhichValue(string name) 
{
	string value = "";
	
	bool nameIsAnInt = true, constantAlreadyExists = false;
	
	for (unsigned int i = 0; i < name.length(); i += 1)
	{
		if (!isdigit(name[i]) && name[i] != '+' && name[i] != '-')
			nameIsAnInt = false;
	}
	
	if (nameIsAnInt || name == "true" || name == "false")
	{
		value = name;
	}
	
	// Name is an identifier and hopefully a constant
	else 
	{
		unsigned int i = 0;
		
		for (i = 0; i < symbolTable.size(); i += 1)
		{
			if (symbolTable[i].externalName == "")
			{
				break;
			}
			else if (symbolTable[i].externalName == name && symbolTable[i].mode == CONSTANT)
			{
				constantAlreadyExists = true;
				break;
			}
		}
		
		//if (constantAlreadyExists && symbolTable[i].value != "")
		if (constantAlreadyExists)
			value = symbolTable[i].value;
		else
			Error("reference to undefined constant");
	}
	
	return value;
}

// Tells which mode a name is in.
modes WhichMode(string name)
{
	modes returnModes;
	
	for (unsigned int i = 0; i < symbolTable.size(); i += 1)
	{
		if (symbolTable[i].externalName == "")
		{
			Error("reference to undefined constant");
		}
		else if (symbolTable[i].externalName == name)
		{
			returnModes = symbolTable[i].mode;
			break;
		}
	}
	
	return returnModes;
}

// Returns the index of a symbol from the external name
int FindIndex(string name)
{
	int returnIndex;
	
	for (unsigned int i = 0; i < symbolTable.size(); i += 1)
	{
		if (symbolTable[i].externalName == "")
		{
			Error("reference to undefined constant");
		}
		else if (symbolTable[i].externalName == name)
		{
			returnIndex = i;
			break;
		}
	}
	
	return returnIndex;
}

// Finds the index of a boolean that has a value of true.
// Returns -1 if no such boolean exists.
int FindIndexOfTrue()
{
	for (unsigned int i = 0; i < symbolTable.size(); i += 1)
	{
		if (symbolTable[i].dataType == BOOLEAN && symbolTable[i].value == "1")
		{
			return i;
		}
	}
	
	return -1;
}

// Finds the index of a boolean that has a value of false.
// Returns -1 if no such boolean exists.
int FindIndexOfFalse()
{
	for (unsigned int i = 0; i < symbolTable.size(); i += 1)
	{
		if (symbolTable[i].dataType == BOOLEAN && symbolTable[i].value == "0")
		{
			return i;
		}
	}
	
	return -1;
}

// Returns the next token or end of file marker.
string NextToken()
{
	token = "";
	while (token == "")
	{
		// Process comment
		if (charac == '{')
		{
			charac = NextChar();
			while (charac != END_OF_FILE && charac != '}')
			{
				charac = NextChar();
			}
			
			if (charac == END_OF_FILE)
			{
				Error("unexpected end of file.");
			}
			else
				NextChar();
		}
		// Process error ('}' at start of token)
		else if (charac == '}')
		{
			Error("\'}\' cannot begin token");
		}
		// Process spaces
		else if (isspace(charac))
		{
			NextChar();
		}
		// Process special characters
		else if (charac == '=' || 
				 charac == ',' || 
				 charac == ';' || 
				 charac == '+' || 
				 charac == '-' || 
				 charac == '.' || 
				 charac == '*' ||
				 charac == '(' ||
				 charac == ')')
		{
			token = charac;
			NextChar();
		}
		else if (charac == ':')
		{
			token = charac;
			NextChar();
			if (charac == '=')
			{
				token += charac;
				NextChar();
			}
		}
		else if (charac == '<')
		{
			token = charac;
			NextChar();
			if (charac == '>' || charac == '=')
			{
				token += charac;
				NextChar();
			}
		}
		else if (charac == '>')
		{
			token = charac;
			NextChar();
			if (charac == '=')
			{
				token += charac;
				NextChar();
			}
		}
		// Process token
		else if (islower(charac))
		{
			token = charac;
			charac = NextChar();
			while (islower(charac) || isdigit(charac) || charac == '_')
			{
				token += charac;
				charac = NextChar();
			}

			if (token[token.length() - 1] == '_')
			{
				Error("\'_\' cannot end token");
			}
		}
		// Process number
		else if (isdigit(charac))
		{
			token = charac;
			while (isdigit(NextChar()))
				token += charac;
		}
		// Process end of file character
		else if (charac == END_OF_FILE)
		{
			token = charac;
		}
		// Otherwise the character is an illegal symbol
		else
		{
			Error("illegal symbol");
		}
	}
	
	return token;
}

// Returns the next character or end of file marker.
char NextChar()
{
	if (!sourceFile.get(charac))
	{
		charac = END_OF_FILE;
	}
	
	// Print to listing file (starting new line if necessary)
	if (lineNumber == 0 && charac == END_OF_FILE)
	{
		lineNumber += 1;
		listingFile << right << setw(5) << lineNumber << "|";
		return charac;
	}
	
	if (lineNumber == 0)
	{
		lineNumber += 1;
		listingFile << right << setw(5) << lineNumber << "|";
	}
	
	if (charac != END_OF_FILE)
	{
		if (printLineNumber)
		{
			lineNumber += 1;
			listingFile << right << setw(5) << lineNumber << "|";
			printLineNumber = false;
		}
		
		if (charac == '\n')
		{
			printLineNumber = true;
		}
		
		listingFile << charac;
	}
	
	return charac;
}

// Print error message to listing
void Error(string errorMessage)
{
	listingFile << "\nError: Line " << lineNumber << ": " << errorMessage;
	listingFile << "\n\nCOMPILATION TERMINATED      1 ERRORS ENCOUNTERED\n";
	
	exit(1);
}

// Check to make sure the token starts with a lowercase letter and
// is only composed of lowercase letters, numbers, and underscores.
bool CheckNonKeyID(string currentToken)
{
	if (CheckForKeyword(currentToken))
		return false;
	if (!islower(currentToken[0]))
		return false;
	for (unsigned int i = 1; i < currentToken.length(); i += 1)
	{
		if (!islower(currentToken[i]) && !isdigit(currentToken[i]) && currentToken[i] != '_')
			return false;
	}
	
	return true;
}

// Check if a word is a keyword
bool CheckForKeyword(string word)
{
	if (word == "program" ||
		word == "begin"   ||
		word == "end"     ||
		word == "var"     ||
		word == "const"   ||
		word == "integer" ||
		word == "boolean" ||
		word == "true"    ||
		word == "false"   ||
		word == "not"	  ||
		word == "mod"	  ||
		word == "div"	  ||
		word == "and"	  ||
		word == "or"	  ||
		word == "read"	  ||
		word == "write"	  ||
		word == "if"	  ||
		word == "then"	  ||
		word == "else"	  ||
		word == "repeat"  ||
		word == "while"	  ||
		word == "do"	  ||
		word == "until")
		return true;
		
	return false;
}

// Checks if a token is a relational operator.
bool CheckForRelationalOperator(string oper_ator)
{
	if (oper_ator == "="  ||
		oper_ator == "<>" ||
		oper_ator == "<=" ||
		oper_ator == ">=" ||
		oper_ator == "<"  ||
		oper_ator == ">")
		return true;
		
	return false;
}

// Checks if a token is a addition-level operator.
bool CheckForAddLevOperator(string oper_ator)
{
	if (oper_ator == "+"  ||
		oper_ator == "-"  ||
		oper_ator == "or")
		return true;
		
	return false;
}

// Checks if a token is a multiplication-level operator.
bool CheckForMultLevOperator(string oper_ator)
{
	if (oper_ator == "*"   ||
		oper_ator == "div" ||
		oper_ator == "mod" ||
		oper_ator == "and")
		return true;
		
	return false;
}

bool IsTokenAnInt(string name)
{
	for (unsigned int i = 0; i < name.length(); i += 1)
	{
		if (!isdigit(name[i]) && name[i] != '+' && name[i] != '-')
			return false;
		if (i > 0 && (name[i] == '+' && name[i] == '-'))
			return false;
	}
	
	return true;
}

bool IsTokenABool(string name)
{
	if (name == "true" || name == "false")
	{
		return true;
	}
	
	return false;
}

bool IsNameInSymbolTable(string name)
{
	unsigned int i = 0;
		
	for (i = 0; i < symbolTable.size(); i += 1)
	{
		if (symbolTable[i].externalName == "")
			return false;
		else if (symbolTable[i].externalName == name)
		{
			return true;
		}
	}
	
	return false;
}

bool CheckForTempName(string name)
{
	if (name[0] != 'T')
		return false;
	for (unsigned int i = 1; i < name.length(); i += 1)
	{
		if (!isdigit(name[i]))
			return false;
	}
	
	return true;
}

// Generate an internal name based on data type and current count of that data type
string GenInternalName(storeType type)
{
	string internalName;
	if (type == PROG_NAME)
		internalName = "P0";
	else if (type == INTEGER)
	{
		internalName = "I" + to_string(integerCount);
		integerCount += 1;
	}
	else if (type == BOOLEAN)
	{
		internalName = "B" + to_string(booleanCount);
		booleanCount += 1;
	}
	
	return internalName;
}
