/**************************************************************************
 
 The MIT License (MIT)
 
 Copyright (c) 2015 Dmitry Sovetov
 
 https://github.com/dmsovetov
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 **************************************************************************/

#include "CgParser.h"
#include "Ast/AstVisitor.h"

#define newAst(T, ...) new (m_allocator.allocate(sizeof(T))) T(__VA_ARGS__)

DC_BEGIN_DREEMCHEST

namespace Renderer
{
    
namespace Cg
{

// ---------------------------------------------- Parser::DeclarationResolver --------------------------------------------- //

//! Resolves declarations used by expression variable terms.
class Parser::DeclarationResolver : public ExpressionVisitor
{
public:

                        //! Constructs an DeclarationResolver instance.
                        DeclarationResolver(Scope& scope, Parser& parser);

    //! Resolves a variable referenced by a term node.
    virtual void        visit(VariableTerm& node) NIMBLE_OVERRIDE;

    //! Resolves a function referenced by a term node.
    virtual void        visit(FunctionCall& node) NIMBLE_OVERRIDE;
    
    //! Blocks a variable resolution process for '.' operator.
    virtual void        visit(Operator& node) NIMBLE_OVERRIDE;

private:

    Scope&              m_scope;    //!< A topmost declaration scope to be used to lookup variables.
    Parser&             m_parser;   //!< A parent parser.
};

// ** Parser::DeclarationResolver::DeclarationResolver
Parser::DeclarationResolver::DeclarationResolver(Scope& scope, Parser& parser)
    : m_scope(scope)
    , m_parser(parser)
{

}

// ** Parser::DeclarationResolver::visit
void Parser::DeclarationResolver::visit(VariableTerm& node)
{
    const Declaration* declaration = m_scope.findInScopeChain(node.name());

    if (!declaration)
    {
        m_parser.emitError(node.line(), node.column(), "undeclared identifier '%s'", node.name().str().c_str());
    }
    else if (declaration->declarationType() == Declaration::VariableDeclaration)
    {
        node.setVariable(static_cast<const Variable*>(declaration));
    }
    else
    {
        m_parser.emitError(node.line(), node.column(), "variable identifier expected instead of '%s'", node.name().str().c_str());
    }
}

// ** Parser::DeclarationResolver::visit
void Parser::DeclarationResolver::visit(FunctionCall& node)
{
    const Declaration* declaration = m_scope.findInScopeChain(node.name());

    if (!declaration)
    {
        m_parser.emitError(node.line(), node.column(), "undeclared identifier '%s'", node.name().str().c_str());
    }
    else if (declaration->declarationType() == Declaration::FunctionDeclaration)
    {
        node.setFunction(static_cast<const Function*>(declaration));
    }
    else
    {
        m_parser.emitError(node.line(), node.column(), "'%s' is not a function", node.name().str().c_str());
    }
}

// ** Parser::DeclarationResolver::visit
void Parser::DeclarationResolver::visit(Operator& node)
{
    // Process all operators other than '.' as always
    if (node.type() != OpMember)
    {
        ExpressionVisitor::visit(node);
        return;
    }

    NIMBLE_ABORT_IF(node.lhs() == NULL, "invalid lhs operator");
    NIMBLE_ABORT_IF(node.rhs() == NULL, "invalid rhs operator");

    // So this is a member access operator, this means that on a left hand side
    // we have a target object and a field on a right hand side.

    // First process a left hand side
    node.lhs()->accept(*this);

    // Now resolve a member by name

    // Get the lhs side type and the rhs side variable
    const Type*   lhs = node.lhs()->type();
	VariableTerm* obj = node.lhs()->isVariable();
    VariableTerm* rhs = node.rhs()->isVariable();

    NIMBLE_ABORT_IF(lhs == NULL, "unhandled lhs type");
	NIMBLE_ABORT_IF(obj == NULL, "lhs is not a structure");

    // Is this a variable term on a right hand side?
    if (rhs == NULL)
    {
        m_parser.emitError(rhs->line(), rhs->column(), "field name expected");
        return;
    }

    // Get the field name
    const StringView& name = rhs->name();
    const Structure* structure = lhs->structure();
    const Variable* field = structure ? static_cast<const Variable*>(structure->declarations().find(name)) : NULL;

    if (structure == NULL)
    {
        m_parser.emitError(rhs->line(), rhs->column(), "'%s' is not a member of '%s'", name.str().c_str(), lhs->name().str().c_str());
        return;
    }

    // Finally set the referenced variable.
    rhs->setVariable(field);

	// Any member of an input structure is also an input.
	if (obj->flags().is(VariableInput))
	{
		rhs->flags().on(VariableInput);
	}

    // And the resulting expression type
    node.setType(&field->type());
}

// ---------------------------------------------------------- Parser ---------------------------------------------------------- //
    
// ** Parser::s_operators
Parser::OperatorInfo Parser::s_operators[TotalOperatorTypes + 1] =
{
      { OpPlusEqual,         0, OpFlagBinary | OpFlagRight | OpFlagLeftLValue }
    , { OpMinusEqual,        0, OpFlagBinary | OpFlagRight | OpFlagLeftLValue }
    , { OpDevideEqual,       2, OpFlagBinary | OpFlagRight | OpFlagLeftLValue }
    , { OpMultiplyEqual,     2, OpFlagBinary | OpFlagRight | OpFlagLeftLValue }
    , { OpEqual,             3, OpFlagBinary | OpFlagRight | OpFlagLeftLValue }
    , { OpPlus,              4, OpFlagBinary }
    , { OpMinus,             4, OpFlagBinary }
    , { OpDivide,            5, OpFlagBinary }
    , { OpMultiply,          5, OpFlagBinary }
    , { OpCompare,           6, OpFlagBinary }
    , { OpLess,              7, OpFlagBinary }
    , { OpLessEqual,         7, OpFlagBinary }
    , { OpGreater,           7, OpFlagBinary }
    , { OpGreaterEqual,      7, OpFlagBinary }
    , { OpMember,            8, OpFlagBinary }
    , { TotalOperatorTypes, -1, 0            }
};

// ** Parser::Parser
Parser::Parser(LinearAllocator& allocator)
    : ExpressionParser(m_tokenizer, 2)
    , m_allocator(allocator)
    , m_tokenizer(allocator)
{
    m_tokenizer.addKeyword("void", TokenBuiltInType, TypeVoid);
    m_tokenizer.addKeyword("float", TokenBuiltInType, TypeFloat);
    m_tokenizer.addKeyword("float2", TokenBuiltInType, TypeFloat2);
    m_tokenizer.addKeyword("float3", TokenBuiltInType, TypeFloat3);
    m_tokenizer.addKeyword("float4", TokenBuiltInType, TypeFloat4);
    m_tokenizer.addKeyword("float4x4", TokenBuiltInType, TypeFloat4x4);
    m_tokenizer.addKeyword("struct", TokenStruct);
    m_tokenizer.addKeyword("if", TokenIf);
    m_tokenizer.addKeyword("else", TokenElse);
    m_tokenizer.addKeyword("while", TokenWhile);
    m_tokenizer.addKeyword("for", TokenFor);
    m_tokenizer.addKeyword("discard", TokenDiscard);
    m_tokenizer.addKeyword("return", TokenReturn);
    m_tokenizer.addKeyword("register", TokenRegister);
    m_tokenizer.addKeyword("NORMAL", TokenInputSemantic);
    m_tokenizer.addKeyword("POSITION", TokenInputSemantic);
    m_tokenizer.addKeyword("TEXCOORD0", TokenInputSemantic);
    m_tokenizer.addKeyword("COLOR", TokenInputSemantic);
    m_tokenizer.addKeyword("COLOR0", TokenOutputSemantic);
    m_tokenizer.addKeyword("COLOR1", TokenOutputSemantic);

    m_tokenizer.addKeyword("#pragma", TokenPreprocessorPragma);
    m_tokenizer.addKeyword("#define", TokenPreprocessorDefine);
    m_tokenizer.addKeyword("#if", TokenPreprocessorIf);
    m_tokenizer.addKeyword("#elif", TokenPreprocessorElif);
    m_tokenizer.addKeyword("#else", TokenPreprocessorElse);
    m_tokenizer.addKeyword("#endif", TokenPreprocessorEndif);

    m_tokenizer.addPunctuation(";", TokenSemicolon);
    m_tokenizer.addPunctuation(":", TokenColon);
    m_tokenizer.addPunctuation("{", TokenBraceOpen);
    m_tokenizer.addPunctuation("}", TokenBraceClose);
    m_tokenizer.addPunctuation("(", TokenParenthesesOpen);
    m_tokenizer.addPunctuation(")", TokenParenthesesClose);
    m_tokenizer.addPunctuation(",", TokenComma);
    
    m_tokenizer.addOperator("+", OpPlus);
    m_tokenizer.addOperator("+=", OpPlusEqual);
    m_tokenizer.addOperator("=", OpEqual);
    m_tokenizer.addOperator(".", OpMember);
    m_tokenizer.addOperator("-", OpMinus);
    m_tokenizer.addOperator("/", OpDivide);
    m_tokenizer.addOperator("*", OpMultiply);
    m_tokenizer.addOperator("<", OpLess);

    registerSemantic("NORMAL", NULL, NORMAL);
    registerSemantic("POSITION", NULL, POSITION);
    registerSemantic("COLOR", NULL, COLOR);
    registerSemantic("COLOR0", NULL, COLOR0);
    registerSemantic("COLOR1", NULL, COLOR1);
    registerSemantic("TEXCOORD0", NULL, TEXCOORD0);
    
    registerSemantic("TEXUNIT0", "s0", TEXUNIT0);
    registerSemantic("TEXUNIT1", "s1", TEXUNIT1);
    registerSemantic("TEXUNIT2", "s2", TEXUNIT2);
    registerSemantic("TEXUNIT3", "s3", TEXUNIT3);
    registerSemantic("TEXUNIT4", "s4", TEXUNIT4);
    registerSemantic("TEXUNIT5", "s5", TEXUNIT5);
    registerSemantic("TEXUNIT6", "s6", TEXUNIT6);
    registerSemantic("TEXUNIT7", "s7", TEXUNIT7);
}

// ** Parser::messages
const OutputMessages& Parser::messages() const
{
    return m_messages;
}
    
// ** Parser::expect
void Parser::expect(TokenType type)
{
    if (check(type))
    {
        next();
        return;
    }
    
    const s8* token = "";
    
    switch (type)
    {
        case TokenSemicolon:
            token = ";";
            break;
        case TokenBraceOpen:
            token = "{";
            break;
        case TokenBraceClose:
            token = "}";
            break;
        case TokenParenthesesOpen:
            token = "(";
            break;
        case TokenParenthesesClose:
            token = ")";
            break;
        default:
            NIMBLE_BREAK;
    }

    emitExpected(token);
    next();
}

// ** Parser::registerBuiltIns
void Parser::registerBuiltIns(Scope& scope)
{
    registerBuiltInType(scope, "bool", TypeBool);
    registerBuiltInType(scope, "int", TypeInt);
    registerBuiltInType(scope, "float", TypeFloat);
    registerBuiltInType(scope, "float2", TypeFloat2);
    registerBuiltInType(scope, "float3", TypeFloat3);
    registerBuiltInType(scope, "float4", TypeFloat4);
    registerBuiltInType(scope, "float4x4", TypeFloat4x4);
}

// ** Parser::registerBuiltInType
void Parser::registerBuiltInType(Scope& scope, const s8* name, BuiltInType builtInType)
{
    // Allocate type identifier
    const Identifier* identifier = internString(name);

    // Allocate type
    Type* type = newAst(Type, *identifier, builtInType, 0, 0);

    // Declare built-in type constructor
    Function* construct = newAst(Function, &scope, *identifier, type, builtInType);

    // Finally register this type
    scope.add(construct);
}
    
// ** Parser::parseProgramSource
Program* Parser::parseProgramSource(const s8* input)
{
    prepare(input);
    
    // Allocate a program instance
    Program* program = newAst(Program);

    // Declare built-in identifiers
    registerBuiltIns(program->scope());

    // Push program root scope onto the stack
    pushDeclarationScope(program->scope());
    
    // Parse program source until the end of a file
    while (hasTokens())
    {
        const Token& token = current();
        
        switch (token.type())
        {
            case TokenIdentifier:
            case TokenBuiltInType:
                if (check(TokenIdentifier, 1) && check(TokenParenthesesOpen, 2))
                {
                    program->addDeclaration(parseFunctionDeclaration());
                }
                else
                {
                    program->addDeclaration(parseVariableDeclaration());
                    expect(TokenSemicolon);
                }
                break;
            case TokenStruct:
                program->addDeclaration(parseStructure());
                break;
            case TokenPreprocessorPragma:
                parsePragma(program);
                break;
            default:
                emitError("unexpected token '%s'", token.str().c_str());
                next();
        }
    }

    // Pop program scope
    popDeclarationScope();

    // Setup shader entry-point functions
    const s8* k_shaderName[] =
    {
          "vertex"
        , "fragment"
        , "geometry"
        , "hull"
        , "domain"
    };

    for (s32 i = 0; i < TotalShaderTypes; i++)
    {
        ShaderType        shader = static_cast<ShaderType>(i);
        const Identifier* name   = program->functionForShader(shader);

        if (name == NULL)
        {
            continue;
        }

        const Declaration* declaration = program->scope().find(name->value());

        if (declaration == NULL)
        {
            emitError(name->line(), name->column(), "undeclared %s shader function '%s'", k_shaderName[shader], name->value().str().c_str());
            continue;
        }
        else if (declaration->declarationType() != Declaration::FunctionDeclaration)
        {
            emitError(name->line(), name->column(), "'%s' is not a valid %s shader function", name->value().str().c_str(), k_shaderName[shader]);
            continue;
        }

        const_cast<Function*>(static_cast<const Function*>(declaration))->setShader(shader);
    }

    // Sort recorded messages
    m_messages.sort();
    
    return program;
}

// ** Parser::parsePragma
void Parser::parsePragma(Program* program)
{
    expect(TokenPreprocessorPragma);

    Token option = read();

    if (option == "vertex")
    {
        if (Identifier* name = expectIdentifier())
        {
            program->setShaderFunction(VertexShader, name);
        }
    }
    else if (option == "fragment")
    {
        if (Identifier* name = expectIdentifier())
        {
            program->setShaderFunction(FragmentShader, name);
        }
    }

    // Make sure that a pragma statement ends with a new line
    if (option.line() == current().line())
    {
        emitExpected("new line");
    }
}

// ** Parser::parseVariableDeclaration
Variable* Parser::parseVariableDeclaration(u8 flags)
{
    // Type name is expected
    Type* type = expectType();
    
    if (!type)
    {
        return NULL;
    }
    
    // Identifier is expected
    Identifier* identifier = expectIdentifier();
    
    if (!identifier)
    {
        return NULL;
    }
    
    // Parse input semantic
    SemanticType semantic = parseSemantic();
    
    // Parse initializer
    Expression* initializer = NULL;
    
    if (parse("="))
    {
        if (check(TokenBraceOpen))
        {
            initializer = parseObjectInitializer();
        }
        else
        {
            initializer = parseExpression();
        }
    }
    
    // Allocate a variable instance
    Variable* variable = newAst(Variable, *identifier, *type, initializer, semantic, flags);
    addDeclaration(variable);

    return variable;
}

// ** Parser::parseObjectInitializer
ObjectInitializer* Parser::parseObjectInitializer()
{
    // Allocate an initializer instance
    ObjectInitializer* initializer = newAst(ObjectInitializer, current().line(), current().column());

    // Starts from a '{' token
    expect(TokenBraceOpen);

    while (!check(TokenBraceClose))
    {
        initializer->addFieldInitializer(parseExpression());
    }

    // Ends with a '}' token
    expect(TokenBraceClose);

    return initializer;
}
    
// ** Parser::parseFunctionDeclaration
Function* Parser::parseFunctionDeclaration()
{
    // Type name is expected
    Type* type = expectType();
    
    if (!type)
    {
        return NULL;
    }
    
    // Identifier is expected
    Identifier* identifier = expectIdentifier();
    
    if (!identifier)
    {
        return NULL;
    }
    
    // Allocate function instance
    Function* function = newAst(Function, scope(), *identifier, type);
    addDeclaration(function);

    // Push function declaration scope
    pushDeclarationScope(function->declarations());
    
    // Parse function arguments
    expect(TokenParenthesesOpen);
    if (!parse(TokenParenthesesClose))
    {
        do
        {
            function->addArgument(parseVariableDeclaration(VariableArgument | VariableInput));
        } while (parse(TokenComma));
        expect(TokenParenthesesClose);
    }
    
    // Parse an output semantic type
    SemanticType semantic = parseSemantic();
    function->setSemantic(semantic);
    
    // Parse function body
    StatementBlock* body = parseStatementBlock();
    function->setBody(body);

    // Pop declaration scope
    popDeclarationScope();

    return function;
}
    
// ** Parser::parseStructure
Structure* Parser::parseStructure()
{
    // Structure declaration starts with a 'struct' keyword
    expect(TokenStruct);
    
    // 'struct' keyword is followed by an identifier
    Identifier* identifier = expectIdentifier();
    
    if (!identifier)
    {
        return NULL;
    }
    
    // Allocate a structure instance
    Structure* structure = newAst(Structure, scope(), *identifier);
    addDeclaration(structure);

    // Push declaration scope
    pushDeclarationScope(structure->declarations());
    
    expect(TokenBraceOpen);
    while (!check(TokenBraceClose))
    {
        // Parse variable declaration
        structure->addField(parseVariableDeclaration(VariableField));
        
        // Each field ends with a semicolon
        expect(TokenSemicolon);
    }
    expect(TokenBraceClose);

    // Pop declaration scope
    popDeclarationScope();

    // Structure declarations ends with a ';' token
    expect(TokenSemicolon);
    
    return structure;
}
    
// ** Parser::parseSemantic
SemanticType Parser::parseSemantic()
{
    SemanticType semantic = INVALID_SEMANTIC;
    
    if (parse(TokenColon))
    {
        semantic = expectSemantic();
    }
    
    return semantic;
}
    
// ** Parser::parseRegisterSemantic
SemanticType Parser::parseRegisterSemantic()
{
    // Register semantic starts from a 'register' keyword
    expect(TokenRegister);

    // 'register' keyword is followed by '(' token
    expect(TokenParenthesesOpen);
    
    // Parse the semantic from an identifier
    Token token = current();
    expect(TokenIdentifier);
    
    // Lookup semantic by name
    SemanticType semantic = findSemanticByToken(token);

    if (semantic == INVALID_SEMANTIC)
    {
        emitError("unknown semantic '%s'", token.str().c_str());
    }
    
    // Register semanti ends with a ')' token
    expect(TokenParenthesesClose);
    
    return semantic;
}
    
// ** Parser::parseStatement
Statement* Parser::parseStatement()
{
    const Token& token     = current();
    Statement*   statement = NULL;
    
    switch (token.type())
    {
        case TokenBuiltInType:
            statement = parseVariableDeclaration();
            expect(TokenSemicolon);
            break;
            
        case TokenIf:
            statement = parseIf();
            break;
            
        case TokenWhile:
            statement = parseWhile();
            break;
            
        case TokenFor:
            statement = parseFor();
            break;
            
        case TokenReturn:
            statement = parseReturn();
            break;
            
        case TokenDiscard:
            statement = parseDiscard();
            break;
            
        case TokenBraceOpen:
            statement = parseStatementBlock();
            break;

        case TokenSemicolon:
            next();
            break;
            
        default:
            if (check(TokenIdentifier) && check(TokenIdentifier, 1))
            {
                statement = parseVariableDeclaration();
            }
            else
            {
                statement = parseExpression();
            }
            expect(TokenSemicolon);
    }
    
    return statement;
}
    
// ** Parser::parseStatementBlock
StatementBlock* Parser::parseStatementBlock()
{
    // Save statment position
    s32 line   = current().line();
    u16 column = current().column();
    
    // Statement block starts with a '{' token
    expect(TokenBraceOpen);
    
    // Allocate statement block instance
    StatementBlock* block = newAst(StatementBlock, scope(), line, column);
    
    pushDeclarationScope(block->declarations());
    while (!check(TokenBraceClose))
    {
        block->addStatement(parseStatement());
    }
    popDeclarationScope();
    
    // Statement block ends with a '}' token
    expect(TokenBraceClose);
    
    return block;
}

// ** Parser::parseReturn
Return* Parser::parseReturn()
{
    // Save statment position
    s32 line   = current().line();
    u16 column = current().column();
    
    expect(TokenReturn);
    
    Expression* value = NULL;
    
    if (!parse(TokenSemicolon))
    {
        value = parseExpression();
    }
    expect(TokenSemicolon);
    
    return newAst(Return, value, line, column);
}

// ** Parser::parseDiscard
Discard* Parser::parseDiscard()
{
    // Save statment position
    s32 line   = current().line();
    u16 column = current().column();
    
    expect(TokenDiscard);
    expect(TokenSemicolon);
    
    return newAst(Discard, line, column);
}
    
// ** Parser::parseIf
If* Parser::parseIf()
{
    // Save statment position
    s32 line   = current().line();
    u16 column = current().column();
    
    // Starts with an 'if' token
    expect(TokenIf);
    
    // Then goes a conditional expression
    Expression* condition = expectConditionalExpression();
    
    if (condition == NULL)
    {
        return NULL;
    }
    
    // Then statment body
    Statement* then = parseStatement();
    
    // And an optional else part
    Statement* otherwise = NULL;
    
    if (parse(TokenElse))
    {
        otherwise = parseStatement();
    }
    
    // Finally allocate if node
    If* statement = newAst(If, condition, then, otherwise, line, column);
    
    return statement;
}
    
// ** Parser::parseWhile
While* Parser::parseWhile()
{
    // Save statment position
    s32 line   = current().line();
    u16 column = current().column();
    
    // Starts with a 'while' token
    expect(TokenWhile);
    
    // Then goes a conditional expression
    Expression* condition = expectConditionalExpression();
    
    if (condition == NULL)
    {
        return NULL;
    }
    
    // While loop body
    Statement* body = parseStatement();
    
    // Finally allocate while node
    While* statement = newAst(While, condition, body, line, column);
    
    return statement;
}
    
// ** Parser::parseFor
For* Parser::parseFor()
{
    // Save statment position
    s32 line   = current().line();
    u16 column = current().column();
    
    // Starts with a 'for' token
    expect(TokenFor);
    
    // Now parse a for loop expressions
    expect(TokenParenthesesOpen);
    
    // Parse initial expression
    Expression* initial = NULL;
    
    if (!parse(TokenSemicolon))
    {
        initial = parseExpression();
        expect(TokenSemicolon);
    }
    
    // Parse conditional expression
    Expression* conditional = NULL;
    
    if (!parse(TokenSemicolon))
    {
        conditional = parseExpression();
        expect(TokenSemicolon);
    }
    
    // Parse an increment expression
    Expression* increment = NULL;
    
    if (!parse(TokenSemicolon))
    {
        increment = parseExpression();
        expect(TokenSemicolon);
    }
    
    // Parse for loop body
    Statement* body = parseStatement();
    
    // Allocate for loop node
    For* for_ = newAst(For, initial, conditional, increment, body, line, column);
    
    return for_;
}
    
// ** Parser::parseFunctionCall
FunctionCall* Parser::parseFunctionCall()
{
    Identifier* identifier = expectFunctionIdentifier();
    
    FunctionCall* call = newAst(FunctionCall, *identifier, identifier->line(), identifier->column());
    
    expect(TokenParenthesesOpen);
    do
    {
        call->addArgument(parseExpression());
    } while (parse(TokenComma));
    expect(TokenParenthesesClose);
    
    return call;
}
    
// ** Parser::parseExpression
Expression* Parser::parseExpression(s32 precedence)
{
    // Parse a left hand side expression
    Expression* lhs = parseTerm();

    // Save an operator line and column
    s32 line = current().line();
    u16 column = current().column();

    // Parse a binary operator and rhs if possible
    OperatorInfo op;
    
    while (checkOperator(op))
    {
        // Should we consume the next operator?
        if ((op.flags & OpFlagBinary) == 0 || op.precedence < precedence)
        {
            break;
        }

        // Consume
        next();

        // Calculate next precedence value
        s32 nextPrecedence = op.flags & OpFlagRight ? op.precedence : op.precedence + 1;
        
        // Parse a right hand side expression
        Expression* rhs = parseExpression(nextPrecedence);
        
        // Compose an operator
        lhs = newAst(Operator, op.type, lhs, rhs, line, column);
    }

    // Post-process finished expression
    if (precedence == 0)
    {
        // First resolve declarations used by variable terms
        lhs->accept(DeclarationResolver(*scope(), *this));
    }
    
    return lhs;
}
    
// ** Parser::parseTerm
Expression* Parser::parseTerm()
{
    const Token& token = current();
    Expression*  term  = NULL;

    switch (token.type()) {
        case TokenIdentifier:
            if (check(TokenParenthesesOpen, 1))
            {
                term = parseFunctionCall();
            }
            else
            {
                term = newAst(VariableTerm, token.text(), token.line(), token.column());
                next();
            }
            break;

        case TokenBuiltInType:
            term = parseFunctionCall();
            break;

        case TokenNumber:
            term = newAst(ConstantTerm, token.text(), token.line(), token.column());
            next();
            break;
            
        default:
            emitExpected("term");
            next();
    }
    
    return term;
}

// ** Parser::checkOperator
bool Parser::checkOperator(OperatorInfo& info) const
{
    if (!check(TokenOperator))
    {
        return false;
    }

    // Extract an operator type
    OperatorType op = static_cast<OperatorType>(current().subtype());
    info = s_operators[op];

    return true;
}
    
// ** Parser::expectConditionalExpression
Expression* Parser::expectConditionalExpression()
{
    Expression* expression = NULL;
    
    if (parse(TokenParenthesesOpen))
    {
        expression = parseExpression();
        expect(TokenParenthesesClose);
        
        return expression;
    }
    
    emitExpected("conditional expression");
    return NULL;
}
    
// ** Parser::expectType
Type* Parser::expectType()
{
    Token token = current();
    
    if (parse(TokenBuiltInType))
    {
        return newAst(Type, *newIdentifier(token), static_cast<BuiltInType>(token.subtype()), token.line(), token.column());
    }
    else if(parse(TokenIdentifier))
    {
        Scope* topmost = scope();

        // Lookup a declaration by it's name
        const Declaration* declaration = topmost->findInScopeChain(token.text());

        if (!declaration)
        {
            emitError("undeclared type '%s'", token.text().str().c_str());
        }
        else if (declaration->declarationType() == Declaration::StructureDeclaration)
        {
            return newAst(Type, *newIdentifier(token), static_cast<const Structure*>(declaration), token.line(), token.column());
        }
        else
        {
            emitExpected("type");
            return newAst(Type, *newIdentifier(token), NULL, token.line(), token.column());
        }
    }
    
    emitExpected("type");
    next();
    
    return NULL;
}
    
// ** Parser::expectOperator
OperatorType Parser::expectOperator()
{
    if (check(TokenOperator))
    {
        OperatorType op = static_cast<OperatorType>(current().subtype());
        next();
        return op;
    }
    
    emitExpected("operator");
    return TotalOperatorTypes;
}
    
// ** Parser::expectIdentifier
Identifier* Parser::expectIdentifier()
{
    Token token = current();
    
    if (parse(TokenIdentifier))
    {
        return newIdentifier(token);
    }
    
    emitExpected("identifier");
    next();
    
    return NULL;
}

// ** Parser::expectFunctionIdentifier
Identifier* Parser::expectFunctionIdentifier()
{
    Token token = current();

    if (parse(TokenBuiltInType))
    {
        return newIdentifier(token);
    }
    else if (parse(TokenIdentifier))
    {
        return newIdentifier(token);
    }

    emitExpected("identifier");
    next();

    return NULL;
}
    
// ** Parser::expectSemantic
SemanticType Parser::expectSemantic()
{
    const Token& token = current();
    SemanticType type = INVALID_SEMANTIC;
    
    switch (token.type())
    {
        case TokenInputSemantic:
        case TokenOutputSemantic:
            type = findSemanticByToken(token);

            if (type == INVALID_SEMANTIC)
            {
                emitError("unknown semantic '%s'", token.str().c_str());
            }

            next();
            return type;
            
        case TokenRegister:
            return parseRegisterSemantic();
    }

    emitExpected("input semantic");
    next();

    return type;
}
    
// ** Parser::registerSemantic
void Parser::registerSemantic(const s8* name, const s8* shortName, SemanticType semantic)
{
    m_registerSemantics[String64(name)] = semantic;

    if (shortName)
    {
        m_registerSemantics[String64(shortName)] = semantic;
    }
}

// ** Parser::findSemanticByToken
SemanticType Parser::findSemanticByToken(const Token& token) const
{
    // Lookup semantic by name
    SemanticType semantic = INVALID_SEMANTIC;

    //const s8* name = token.text();
    RegisterSemanticTypes::const_iterator i = m_registerSemantics.find(String64(token.str().c_str()));

    if (i != m_registerSemantics.end())
    {
        semantic = i->second;
    }

    return semantic;
}

// ** Parser::findDeclaration
const Declaration* Parser::findDeclaration(const Identifier* identifier) const
{
    Scope* topmost = scope();
    const Declaration* declaration = topmost->findInScopeChain(identifier->value());
    return declaration;
}

// ** Parser::addDeclaration
void Parser::addDeclaration(const Declaration* declaration)
{
    Scope* topmost = scope();
    const StringView& name = declaration->name();

    if (const Declaration* prev = topmost->find(name))
    {
        emitError("'%s' is already declared on line %d:%d", name.str().c_str(), prev->line(), prev->column());
    }
    else
    {
        topmost->add(declaration);
    }
}

// ** Parser::scope
Scope* Parser::scope() const
{
    NIMBLE_ABORT_IF(m_scopeStack.empty(), "scope stack is empty");
    Scope* scope = m_scopeStack.top();
    return scope;
}

// ** Parser::pushDeclarationScope
void Parser::pushDeclarationScope(Scope& scope)
{
    m_scopeStack.push(&scope);
}

// ** Parser::popDeclarationScope
void Parser::popDeclarationScope()
{
    NIMBLE_ABORT_IF(m_scopeStack.empty(), "stack underflow");
    m_scopeStack.pop();
}

// ** Parser::emitError
void Parser::emitError(const s8* format, ...)
{
    NIMBLE_LOGGER_FORMAT(format);

    s32 size = static_cast<s32>(strlen(buffer)) + 1;
    s8* message = reinterpret_cast<s8*>(m_allocator.allocate(size));
    memcpy(message, buffer, size);

    m_messages.error(current().line(), current().column(), message);
}

// ** Parser::emitError
void Parser::emitError(s32 line, u16 column, const s8* format, ...)
{
    NIMBLE_LOGGER_FORMAT(format);

    s32 size = static_cast<s32>(strlen(buffer)) + 1;
    s8* message = reinterpret_cast<s8*>(m_allocator.allocate(size));
    memcpy(message, buffer, size);

    m_messages.error(line, column, message);
}

// ** Parser::emitExpected
void Parser::emitExpected(const s8* expected)
{
    emitError("expected '%s' after '%s', got '%s'", expected, previous().str().c_str(), current().str().c_str());
}
    
// ** Parser::newIdentifier
Identifier* Parser::newIdentifier(const Token& token)
{
    u8* pointer = m_allocator.allocate(sizeof(Identifier));
    return new (pointer) Identifier(token.text(), token.line(), token.column());
}

// ** Parser::internString
const Identifier* Parser::internString(const s8* value)
{
    s32 length = static_cast<s32>(strlen(value) + 1);

    s8* str = reinterpret_cast<s8*>(m_allocator.allocate(length));
    memcpy(str, value, length);

    Identifier* identifier = newAst(Identifier, StringView(str, length - 1), 0, 0);
    return identifier;
}

// --------------------------------------------------- OutputMessages --------------------------------------------------- //

// ** OutputMessages::OutputMessages
OutputMessages::OutputMessages()
    : m_errors(0)
{

}

// ** OutputMessages::pushMessage
void OutputMessages::pushMessage(MessageType type, s32 line, u16 column, const s8* text)
{
    Message msg;
    msg.line = line;
    msg.column = column;
    msg.type = type;
    msg.text = text;
    m_messages.push_back(msg);
}

// ** OutputMessages::error
void OutputMessages::error(s32 line, u16 column, const s8* text)
{
    m_errors++;
    pushMessage(ErrorMessage, line, column, text);
}

// ** OutputMessages::warning
void OutputMessages::warning(s32 line, u16 column, const s8* text)
{
    pushMessage(WarningMessage, line, column, text);
}

// ** OutputMessages::verbose
void OutputMessages::verbose(s32 line, u16 column, const s8* text)
{
    pushMessage(VerboseMessage, line, column, text);
}

// ** OutputMessages::error
void OutputMessages::sort()
{
    // Local sorting predicate
    struct Predicate
    {
        static bool less(const Message& a, const Message& b)
        {
            if (a.line != b.line)
            {
                return a.line < b.line;
            }

            return a.column < b.column;
        }
    };

    m_messages.sort(Predicate::less);
}

// ** OutputMessages::messages
const OutputMessages::Messages& OutputMessages::messages() const
{
    return m_messages;
}

// ** OutputMessages::errorCount
s32 OutputMessages::errorCount() const
{
    return m_errors;
}
    
} // namespace Cg
    
} // namespace Renderer

DC_END_DREEMCHEST
