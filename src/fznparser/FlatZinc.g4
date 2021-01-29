grammar FlatZinc;

model :
  ( predicateItem )*
  ( parDeclItem )*
  ( varDeclItem )*
  ( constraintItem )*
  solveItem
;

// ===== Predicate items
predicateItem : 'predicate' Identifier '(' predParam ( ',' predParam )* ')' ';' ;

predParam : predParamType ':' Identifier ;

// ===== Identifiers 
basicParType : 'bool'
                   | 'int'
                   | 'float'
                   | 'set of int'
    ;

parType : basicParType
             | 'array' '[' indexSet ']' 'of' basicParType
    ;

basicVarType : 'var' basicParType
                   | 'var' IntLiteral '..' IntLiteral
                   | 'var' '{' IntLiteral (',' IntLiteral )* '}'
                   | 'var' FloatLiteral '..' FloatLiteral
                   | 'var' 'set' 'of' IntLiteral '..' IntLiteral
                   | 'var' 'set' 'of' '{' IntLiteral (',' IntLiteral)* '}'
    ;

arrayVarType : 'array' '[' indexSet ']' 'of' basicVarType ;

// indexSet : '1' '..' IntLiteral ;
indexSet : IntLiteral '..' IntLiteral ;

basicPredParamType : basicParType
                          | basicVarType
                          | IntLiteral '..' IntLiteral
                          | FloatLiteral '..' FloatLiteral
                          | '{' IntLiteral (',' IntLiteral )* '}'
                          | 'set' 'of' IntLiteral '..' IntLiteral
                          | 'set' 'of' '{'   IntLiteral (IntLiteral ',')* '}'
    ;

predParamType : basicPredParamType
                    | 'array' '[' predIndexSet ']' 'of' basicPredParamType
    ;

predIndexSet : indexSet
                   | 'int'
    ;

basicLiteralExpr : boolLiteral
                       | IntLiteral
                       | FloatLiteral
                       | setLiteral
    ;

basicExpr : basicLiteralExpr
              // | VarParIdentifier
    | Identifier
    ;

expr       : basicExpr
               | arrayLiteral
    ;

parExpr   : basicLiteralExpr
               | parArrayLiteral
    ;


// ===== Boolean literals
boolLiteral : 'false'
                 | 'true'
    ;


// ===== Set literals
setLiteral : '{' IntLiteral (',' IntLiteral )* '}'
                | IntLiteral '..' IntLiteral
                | '{' FloatLiteral ( ',' FloatLiteral )* '}'
                | FloatLiteral '..' FloatLiteral
    ;

arrayLiteral : '[' basicExpr ( ',' basicExpr )* ']' ;

parArrayLiteral : '[' basicLiteralExpr (',' basicLiteralExpr )* ']' ;

// ===== Parameter declarations

parDeclItem : parType ':' Identifier '=' parExpr ';' ;
// parDeclItem : parType ':' VarParIdentifier '=' parExpr ';' ;

// ===== Variable declarations

// varDeclItem : basicVarType ':' VarParIdentifier annotations ( '=' basicExpr )? ';'
                  // | arrayVarType ':' VarParIdentifier annotations '=' arrayLiteral ';'
varDeclItem : basicVarType ':' Identifier annotations ( '=' basicExpr )? ';'
                  | arrayVarType ':' Identifier annotations '=' arrayLiteral ';'
    ;


// ===== Constraint items

constraintItem : 'constraint' Identifier '(' expr (',' expr)* ')' annotations ';' ;

// ===== Solve item

solveItem : 'solve' annotations 'satisfy' ';'
               | 'solve' annotations 'minimize' basicExpr ';'
               | 'solve' annotations 'maximize' basicExpr ';'
    ;

// ===== Annotations

annotations : ( '::' annotation )* ;

annotation : Identifier
               | Identifier '(' annExpr (',' annExpr)* ')'
    ;

annExpr   : basicAnnExpr
              | '[' basicAnnExpr (',' basicAnnExpr)* ']'
    ;

basicAnnExpr   : basicLiteralExpr
                   | stringLiteral
                    | annotation
    ;


// ===== Lexer 
Identifier : [A-Za-z][A-Za-z0-9_]* ;

// VarParIdentifier : [A-Za-z_][A-Za-z0-9_]* ;

IntLiteral : [-]?[0-9]+
                | [-]? '0x' [0-9A-Fa-f]+
                | [-]? '0o' [0-7]+
    ;

FloatLiteral : [-]?[0-9]+.[0-9]+
                  | [-]?[0-9]+.[0-9]+[Ee][-+]?[0-9]+
                  | [-]?[0-9]+[Ee][-+]?[0-9]+
    ;

WS : [ \t\r\n]+ -> skip;

stringLiteral: StringLiteral ;
StringLiteral :  '"' ~('"')+ '"' ;
