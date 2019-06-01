/* $Header$ */
/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2017
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 * 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * With the contribution of Ankit Aggarwal <ankit.ankit.aggarwal@gmail.com>
 * during Google Summer of Code 2015
 */

#ifndef MATHP_H
#define MATHP_H

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <cmath>
#include <time.h>
#include <iostream>
#include <vector>
#include <map>
#include <stack>
#include <string>

#include "myassert.h"
#include "mynewmem.h"
#include "except.h"

#include "mathtyp.h"
#include "table.h"
#include "input.h"

#ifndef DO_NOT_USE_EE
#include "evaluator.h"
#endif // DO_NOT_USE_EE

class MathParser {
public:
	enum ArgType {
		/* AT_PRIVATE means only who created that type
		 * is supposed to deal with it (the default);
		 * the MathParser only knows how to deal with
		 * the remaining types */
		AT_PRIVATE,

		AT_TYPE,
		AT_ANY,
		AT_VOID,
		AT_BOOL,
		AT_INT,
		AT_REAL,
		AT_STRING
	};

	enum ArgFlag {
		AF_NONE			= 0x0U,
		AF_OPTIONAL		= 0x1U,
		AF_OPTIONAL_NON_PRESENT	= 0x2U,
		AF_CONST		= 0x10U
	};

	class MathArg_t {
	private:
		unsigned m_flags;

	public:
		MathArg_t(unsigned f = AF_NONE) : m_flags(f) {};
		virtual ~MathArg_t(void) { NO_OP; };

		void SetFlag(const MathParser::ArgFlag& f) { m_flags |= unsigned(f); };
		void ClearFlag(const MathParser::ArgFlag& f) { m_flags &= ~unsigned(f); };
		bool IsFlag(const MathParser::ArgFlag f) const { return (m_flags & unsigned(f)) == unsigned(f); };
		unsigned GetFlags(void) const { return m_flags; };
		virtual ArgType Type(void) const = 0;
#ifndef DO_NOT_USE_EE
		virtual void SetExpr(const ExpressionElement *ee) { throw ErrGeneric(MBDYN_EXCEPT_ARGS); };
		virtual const ExpressionElement *GetExpr(void) const { return 0; };
		virtual void Eval(void) { NO_OP; };
#endif // DO_NOT_USE_EE
		virtual MathArg_t *Copy(void) const = 0;
	};

	class MathArgVoid_t : public MathArg_t {
	public:
		virtual ~MathArgVoid_t(void) { NO_OP; };
		virtual ArgType Type(void) const { return AT_VOID; };
		virtual MathArg_t *Copy(void) const { return new MathArgVoid_t; };
	};

	template <class T, ArgType TT = AT_PRIVATE>
	class MathArgPriv_t : public MathArg_t {
	protected:
		T m_val;
#ifndef DO_NOT_USE_EE
		const ExpressionElement *m_ee; // NOTE: memory owned by someone else
#endif // DO_NOT_USE_EE

	public:
#ifndef DO_NOT_USE_EE
		MathArgPriv_t(const T& val, unsigned f = AF_NONE) : MathArg_t(f), m_val(val), m_ee(0) { NO_OP; };
		MathArgPriv_t(const T& val, const ExpressionElement *ee, unsigned f = AF_NONE) : MathArg_t(f), m_val(val), m_ee(ee) { NO_OP; };
		MathArgPriv_t(void) : MathArg_t(AF_NONE), m_ee(0) { NO_OP; };
#else // ! USE_EE
		MathArgPriv_t(const T& val, unsigned f = AF_NONE) : MathArg_t(f), m_val(val) { NO_OP; };
		MathArgPriv_t(void) : MathArg_t(AF_NONE) { NO_OP; };
#endif // ! USE_EE

		virtual ~MathArgPriv_t(void) { NO_OP; };
		virtual ArgType Type(void) const { return TT; };
#ifndef DO_NOT_USE_EE
		virtual void SetExpr(const ExpressionElement *ee) { m_ee = ee; };
		virtual const ExpressionElement *GetExpr(void) const { return m_ee; };
		virtual void Eval(void) { if (m_ee) { EE_Eval(m_val, m_ee); } };
		virtual MathArg_t *Copy(void) const { return new MathArgPriv_t<T, TT>(m_val, m_ee, GetFlags()); };
#else // ! USE_EE
		virtual MathArg_t *Copy(void) const { return new MathArgPriv_t<T, TT>(m_val, GetFlags()); };
#endif // ! USE_EE

		const T& operator()(void) const { return m_val; };
		T& operator()(void) { return m_val; };
	};

	// not used right now; could be used for casting and so
	typedef MathArgPriv_t<ArgType, AT_TYPE> MathArgType_t;

	typedef MathArgPriv_t<TypedValue, AT_ANY> MathArgAny_t;
	typedef MathArgPriv_t<bool, AT_BOOL> MathArgBool_t;
	typedef MathArgPriv_t<Int, AT_INT> MathArgInt_t;
	typedef MathArgPriv_t<Real, AT_REAL> MathArgReal_t;
	typedef MathArgPriv_t<std::string, AT_STRING> MathArgString_t;

	typedef std::vector<MathArg_t*> MathArgs;
	typedef int (*MathFunc_f)(const MathArgs& args);
	typedef int (*MathFuncTest_f)(const MathArgs& args);

	// forward declaration
	class NameSpace;

	/* struttura delle funzioni built-in */
	struct MathFunc_t {
		std::string fname;		/* function name */
		NameSpace *ns;			/* puntatore a namespace */
		MathArgs args;			/* argomenti (0: out; 1->n: in) */
		MathFunc_f f;			/* puntatore a funzione */
		MathFuncTest_f t;		/* puntatore a funzione di test */
		std::string errmsg;		/* messaggio di errore */

		MathFunc_t(void) : ns(0), f(0), t(0) {};
		MathFunc_t(const MathFunc_t& in) : fname(in.fname), ns(in.ns), args(in.args.size()), f(in.f), t(in.t), errmsg(in.errmsg) {
			for (unsigned i = 0; i < args.size(); ++i) {
				args[i] = in.args[i]->Copy();
			}
		};
		~MathFunc_t(void) {
			for (unsigned i = 0; i < args.size(); ++i) {
				delete args[i];
			}
		};
	};

	/* carattere di inizio commento */
	static const char ONE_LINE_REMARK = '#';

	/* Namespace */
	class NameSpace {
		std::string name;

	public:
		NameSpace(const std::string& name);
		virtual ~NameSpace(void);
		virtual const std::string& sGetName(void) const;
		virtual bool IsFunc(const std::string& fname) const = 0;
		virtual MathParser::MathFunc_t* GetFunc(const std::string& fname) const = 0;
		virtual TypedValue EvalFunc(MathParser::MathFunc_t *f) const = 0;
		virtual Table* GetTable(void) = 0;
	};

	/* Static namespace */
	class StaticNameSpace : public MathParser::NameSpace {
	private:
		typedef std::map<std::string, MathParser::MathFunc_t *> funcType;
		funcType func;
		Table *m_pTable;

	public:
		StaticNameSpace(Table *pTable = 0);
		~StaticNameSpace(void);

		bool IsFunc(const std::string& fname) const;
		MathParser::MathFunc_t* GetFunc(const std::string& fname) const;
		virtual TypedValue EvalFunc(MathParser::MathFunc_t *f) const;
		virtual Table* GetTable(void);
	};

	/* Prototipo dei plugin */
	class PlugIn {
	protected:
		MathParser& mp;
 
	/*
	 * Idea base: 
	 * un plugin e' un qualcosa che viene collegato al parser
	 * dopo la sua costruzione.  Il plugin e' dotato di 'tipo'
	 * (sName).  Quando il parser incontra una parentesi quadra
	 * che si apre ('['), si comporta come segue:
	 *   - esegue il parsing dello stream fino alla chiusura
	 *     della parentesi (']'), 
	 *   - separando i token in base alle virgole (','), 
	 *   - quindi legge il primo token che contiene il nome del
	 *     plugin
	 *   - legge il secondo token con il quale definisce una
	 *     metavariabile a cui il risultato della creazione
	 *     del plugin viene assegnato.
	 * Il risultato e' una metavariabile, a cui e' associata una
	 * certa operazione.  Ogni qualvolta la variabile viene usata,
	 * questo provoca l'esecuzione del plugin.
	 */
	public:
		PlugIn(MathParser& mp) : mp(mp) {};
		virtual ~PlugIn() {};
		virtual const char *sName(void) const = 0;
		virtual int Read(int argc, char *argv[]) = 0;
		virtual TypedValue::Type GetType(void) const = 0;
		virtual TypedValue GetVal(void) const = 0;
	};

protected:
	class PlugInVar : public NamedValue {
	private:
		MathParser::PlugIn *pgin;

	public:
		PlugInVar(const char *const s, MathParser::PlugIn *p);
		~PlugInVar(void);

		TypedValue::Type GetType(void) const;
		bool Const(void) const;
		bool MayChange(void) const;
		TypedValue GetVal(void) const;
	};

	struct PlugInRegister {
		const char *name;
		MathParser::PlugIn * (*constructor)(MathParser&, void *);
		void *arg;
		struct PlugInRegister *next; 

		PlugInRegister(void)
		: name(NULL), constructor(0), arg(0), next(0) {};
	} *PlugIns;

public:

	/* Gestione degli errori */
	class ErrGeneric : public MBDynErrBase {
	public:
		ErrGeneric(MBDYN_EXCEPT_ARGS_DECL);
		ErrGeneric(MathParser* p, MBDYN_EXCEPT_ARGS_DECL);
	};
   
	Table&   table;      /* symbol table */
	bool bRedefineVars;  /* redefine_vars flag */

public:
	/* gioca con table e stream di ingresso */
	Table& GetSymbolTable(void) const;
	void PutSymbolTable(Table& T);

	InputStream* in;     /* stream in ingresso */
	int GetLineNumber(void) const;

public:
   
	/* i token che il lex riconosce */
	enum Token {
		ENDOFFILE = -2,
		UNKNOWNTOKEN = -1,
	
		MP_INT = TypedValue::VAR_INT,
		MP_REAL = TypedValue::VAR_REAL,	
		MP_STRING = TypedValue::VAR_STRING,	
	
		NUM,		/* Numero */
		NAME, 		/* Nome */
		EXP,		/* '^'	: Elevamento a potenza */
		MULT,		/* '*'	: Moltiplicazione */
		DIV,		/* '/'	: Divisione */
		MOD,		/* '%'	: Divisione */
		MINUS,		/* '-'	: Meno */
		PLUS,		/* '+'	: Piu' */
		GT,		/* '>'	: Maggiore di */
		GE,		/* '>='	: Maggiore o uguale */
		EQ,		/* '=='	: Uguale */
		LE,		/* '<='	: Minore o uguale */
		LT,		/* '<'	: Minore di */
		NE,		/* '!='	: Diverso da */
		NOT,		/* '!'	: Negazione (operatore logico) */
		AND,		/* '&&'	: AND (operatore logico) */
		OR,		/* '||'	: OR (operatore logico) */
		XOR,		/* '~|'	: XOR, o OR esclusivo (op. logico) */
		
		OBR,		/* '('	: Parentesi aperta */
		CBR,		/* ')'	: Parentesi chiusa */
		OPGIN,		/* '['	: Apertura di plugin statement */
		CPGIN,		/* ']'	: Chiusura di plugin statement */
		STMTSEP,	/* ';'	: Separatore di statements */
		ARGSEP,		/* ','	: Separatore di argomenti */
		NAMESPACESEP,	/* '::'	: Separatore di namespace */
		ASSIGN,		/* '='	: Assegnazione */

		LASTTOKEN	
	};

   	enum DeclarationModifier {
		DMOD_UNKNOWN = -1,
		DMOD_IFNDEF,
		DMOD_LAST
	};

protected:

	StaticNameSpace* defaultNameSpace;

public:
	typedef std::map<std::string, NameSpace *> NameSpaceMap;
	const NameSpaceMap& GetNameSpaceMap(void) const;

protected:
	NameSpaceMap nameSpaceMap;

	/* buffer statico reallocabile per leggere nomi */
	std::string namebuf;

	/* valore numerico dotato di tipo */
	TypedValue value;

	/* token corrente */
	enum Token currtoken;

	struct TokenVal {
		Token m_t;
		TypedValue m_v;
	};
	std::stack<TokenVal> TokenStack;

	/* operazioni sulla stack */
	void TokenPush(enum Token t);
	int TokenPop(void);

	TypedValue::Type GetType(const char* const s) const;
	TypedValue::TypeModifier GetTypeModifier(const char* const s) const;
	MathParser::DeclarationModifier GetDeclarationModifier(const char* const s) const;

	bool IsType(const char* const s) const;
	bool IsTypeModifier(const char* const s) const;
	bool IsDeclarationModifier(const char* const s) const;
	bool IsKeyWord(NameSpace *ns, const char* const s) const;
   
	/* lexer */
	enum Token GetToken(void);

	void trim_arg(char *const s);

	/*
	 * functions whose recursive invocation expresses
	 * operators' precedence
	 *
	 * NOTE:
	 * - the *_int(d) version does the actual job
	 * - the *(void) version first parses the first operand,
	 *   then calls *_int(d)
	 * - the *(d) version calls *_int(d) with the argument
	 */

#ifndef DO_NOT_USE_EE
	ExpressionElement* logical(void);
	ExpressionElement* logical(ExpressionElement* d);
	ExpressionElement* logical_int(ExpressionElement* d);
	ExpressionElement* relational(void);
	ExpressionElement* relational(ExpressionElement* d);
	ExpressionElement* relational_int(ExpressionElement* d);
	ExpressionElement* binary(void);
	ExpressionElement* binary(ExpressionElement* d);
	ExpressionElement* binary_int(ExpressionElement* d);
	ExpressionElement* mult(void);
	ExpressionElement* mult(ExpressionElement* d);
	ExpressionElement* mult_int(ExpressionElement* d);
	ExpressionElement* power(void);
	ExpressionElement* power(ExpressionElement* d);
	ExpressionElement* power_int(ExpressionElement* d );
	ExpressionElement* unary(void);

	/* helper for expr, which evaluates [built-in] functions */
	ExpressionElement* parsefunc(MathParser::MathFunc_t* f);

	/* evaluates expressions */
	ExpressionElement* expr(void);

	/* evaluates statements and statement lists */
	ExpressionElement* stmt(void);
	ExpressionElement* stmtlist(void);

	ExpressionElement* readplugin(void);

#else // ! USE_EE
	TypedValue logical(void);
	TypedValue logical(TypedValue d);
	TypedValue logical_int(TypedValue d);
	TypedValue relational(void);
	TypedValue relational(TypedValue d);
	TypedValue relational_int(TypedValue d);
	TypedValue binary(void);
	TypedValue binary(TypedValue d);
	TypedValue binary_int(TypedValue d);
	TypedValue mult(void);
	TypedValue mult(TypedValue d);
	TypedValue mult_int(TypedValue d);
	TypedValue power(void);
	TypedValue power(TypedValue d);
	TypedValue power_int(TypedValue d);
	TypedValue unary(void);

	/* helper per expr, che valuta le funzioni built-in */
	TypedValue evalfunc(MathParser::NameSpace *ns, MathParser::MathFunc_t* f);

	/* evaluates expressions */
	TypedValue expr(void);

	/* evaluates statements and statement lists */
	TypedValue stmt(void);
	TypedValue stmtlist(void);

	TypedValue readplugin(void);
#endif // ! USE_EE

private:
	// no copy constructor
	MathParser(const MathParser&);

public:   
	MathParser(const InputStream& strm, Table& t, bool bRedefineVars = false);
	MathParser(Table& t, bool bRedefineVars = false);
	~MathParser(void);

	NamedValue *
	InsertSym(const char* const s, const Real& v, int redefine = 0);
	NamedValue *
	InsertSym(const char* const s, const Int& v, int redefine = 0);   
     
	/*
	 * interpreta una sequenza di stmt e restituisce il valore
	 * dell'ultimo
	 */
	Real GetLastStmt(Real d = 0., Token t = ARGSEP);
	Real GetLastStmt(const InputStream& strm, Real d = 0.,
			Token t = ARGSEP);

#ifndef DO_NOT_USE_EE
	/* interpreta uno stmt e ne restitutisce il valore */
	ExpressionElement *GetExpr(void);
	ExpressionElement *GetExpr(const InputStream& strm);
#endif // DO_NOT_USE_EE

	Real Get(Real d = 0.);
	Real Get(const InputStream& strm, Real d = 0.);
	TypedValue Get(const TypedValue& v);
	TypedValue Get(const InputStream& strm, const TypedValue& v);

	/* modalita' calcolatrice: elabora e stampa ogni stmt */
	void GetForever(std::ostream& out, const char* const sep = "\n");
	void GetForever(const InputStream& strm, std::ostream& out,
			const char* const sep = "\n");

	int RegisterPlugIn(const char *name,
			MathParser::PlugIn * (*)(MathParser&, void *),
			void *arg);

	int RegisterNameSpace(NameSpace *ns);

	NameSpace *GetNameSpace(const std::string& name) const;

	/* validates a name */
	bool bNameValidate(const std::string& s) const;
};

extern std::ostream&
operator << (std::ostream& out, const MathParser::MathArgVoid_t& v);
extern std::ostream&
operator << (std::ostream& out, const MathParser::MathArgBool_t& v);
extern std::ostream&
operator << (std::ostream& out, const MathParser::MathArgInt_t& v);
extern std::ostream&
operator << (std::ostream& out, const MathParser::MathArgReal_t& v);
extern std::ostream&
operator << (std::ostream& out, const MathParser::MathArgString_t& v);

#endif /* MATHP_H */

