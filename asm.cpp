#include<QStringList>

#include <QDebug>

#include "asm.h"
#include "code.h"

// Regular expressions for lexical analysis
QRegExp Asm::rxComment("^//.*");
QRegExp Asm::rxDigit("^[0-9]+");
QRegExp Asm::rxIdentifier("^((([A-Z|a-z]{1})(\\w*))(:){0,1})");
QRegExp Asm::rxHexConst("^((0(?![x|X]))|((0)([x|X])([0-9|A-F|a-f])+)|((0)([0-9]+)))");

bool Asm::getToken(QString &sourceLine, ELexicalToken &token, QString &tokenString)
{
    sourceLine = sourceLine.trimmed();
    if (sourceLine.length() == 0) {
        token = LT_EMPTY;
        tokenString = "";
        return true;
    }
    QChar firstChar = sourceLine[0];
    if (firstChar == ',') {
        token = LT_COMMA;
        tokenString = ",";
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    if (firstChar == '[') {
        token = LT_LEFT_BRACKET;
        tokenString = "[";
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    if (firstChar == ']') {
        token = LT_RIGHT_BRACKET;
        tokenString = "]";
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    if (firstChar == '/') {
        if (rxComment.indexIn(sourceLine) == -1) {
            tokenString = "//ERROR: Malformed comment"; // Should occur with single "/".
            return false;
        }
        token = LT_COMMENT;
        tokenString = rxComment.capturedTexts()[0];
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    if (startsWithHexPrefix(sourceLine)) {
        if (rxHexConst.indexIn(sourceLine) == -1) {
            tokenString = ";ERROR: Malformed hex constant.";
            return false;
        }
        token = LT_HEX_CONSTANT;
        tokenString = rxHexConst.capturedTexts()[0];
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    if (firstChar.isDigit()) {
        if (rxDigit.indexIn(sourceLine) == -1) {
            tokenString = "//ERROR: Malformed integer"; // Should not occur.
            return false;
        }
        token = LT_DIGIT;
        tokenString = rxDigit.capturedTexts()[0];
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    if (firstChar == '=') {
        token = LT_EQUALS;
        tokenString = "=";
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    if (firstChar.isLetter()) {
        if (rxIdentifier.indexIn(sourceLine) == -1) {
            tokenString = "//ERROR: Malformed identifier"; // Should not occur
            return false;
        }
        token = LT_IDENTIFIER;
        token = tokenString.endsWith(':') ? LT_PRE_POST : LT_IDENTIFIER;
        tokenString = rxIdentifier.capturedTexts()[0];
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    if (firstChar == ';') {
        token = LT_SEMICOLON;
        tokenString = ";";
        sourceLine.remove(0, tokenString.length());
        return true;
    }
    tokenString = "//ERROR: Syntax error starting with " + QString(firstChar);
    return false;
}

bool Asm::processSourceLine(QString sourceLine, Code *&code, QString &errorString)
{
    Asm::ELexicalToken token; // Passed to getToken.
    QString tokenString; // Passed to getToken.
    QString localIdentifier = ""; // Saves identifier for processing in the following state.
    Enu::EMnemonic localEnumMnemonic; // Key to Pep:: table lookups.
    bool processingPrecondition; // To distinguish between a precondition and a postcondition.

    // The concrete code objects asssigned to code.
    MicroCode *microCode = NULL;
    CommentOnlyCode *commentOnlyCode = NULL;
    PreconditionCode *preconditionCode = NULL;
    PostconditionCode *postconditionCode = NULL;
    BlankLineCode *blankLineCode = NULL;

    Asm::ParseState state = Asm::PS_START;
    do {
        if (!getToken(sourceLine, token, tokenString)) {
            errorString = tokenString;
            return false;
        }
        switch (state) {
        case Asm::PS_START:
            if (token == Asm::LT_IDENTIFIER) {
                if (Pep::mnemonToDecControlMap.contains(tokenString.toUpper())) {
                    microCode = new MicroCode();
                    code = microCode;
                    localEnumMnemonic = Pep::mnemonToDecControlMap.value(tokenString.toUpper());
                    localIdentifier = tokenString;
                    state = Asm::PS_EQUAL_DEC;
                }
                else if (Pep::mnemonToMemControlMap.contains(tokenString.toUpper())) {
                    microCode = new MicroCode;
                    code = microCode;
                    localEnumMnemonic = Pep::mnemonToMemControlMap.value(tokenString.toUpper());
                    microCode->set(localEnumMnemonic, 1);
                    state = Asm::PS_CONTINUE_PRE_SEMICOLON;
                }
                else if (Pep::mnemonToClockControlMap.contains(tokenString.toUpper())) {
                    errorString = "//ERROR: Clock signal " + tokenString + " must appear after semicolon";
                    return false;
                }
                else {
                    errorString = "//ERROR: Unrecognized control signal: " + tokenString;
                    return false;
                }
            }
            else if (token == Asm::LT_SEMICOLON) {
                errorString = "//ERROR: No control signals before semicolon.";
                return false;
            }
            else if (token == Asm::LT_COMMENT) {
                blankLineCode = new BlankLineCode();
                code = blankLineCode;
                state = Asm::PS_COMMENT;
            }
            else if (token == Asm::LT_PRE_POST) {
                if (Pep::mnemonToSpecificationMap.contains(tokenString.toUpper())) {
                    if (Pep::mnemonToSpecificationMap.value(tokenString.toUpper()) == Enu::E_Pre) {
                        processingPrecondition = true;
                        preconditionCode = new PreconditionCode();
                        code = preconditionCode;
                        state = PS_START_SPECIFICATION;
                    }
                    else { // E_Post
                        processingPrecondition = false;
                        postconditionCode = new PostconditionCode();
                        code = postconditionCode;
                        state = PS_START_SPECIFICATION;
                    }
                }
                else {
                    errorString = "//ERROR: Illegal specification symbol " + tokenString;
                    return false;
                }
            }
            else if (token == Asm::LT_EMPTY) {
                commentOnlyCode = new CommentOnlyCode(tokenString);
                code = commentOnlyCode;
                state = Asm::PS_FINISH;
            }
            else {
                errorString = "//ERROR: Syntax error where control signal or comment expected";
                return false;
            }
            break;

        case Asm::PS_EQUAL_DEC:
            if (token == Asm::LT_EQUALS) {
                state = Asm::PS_DEC_CONTROL;
            }
            else {
                errorString = "//ERROR: Expected = after " + localIdentifier;
                delete code;
                return false;
            }
            break;

        case Asm::PS_DEC_CONTROL:
            if (token == Asm::LT_DIGIT) {
                if (microCode->has(localEnumMnemonic)) {
                    errorString = "//ERROR: Duplicate control signal, " + localIdentifier;
                    delete code;
                    return false;
                }
                bool ok;
                int value = tokenString.toInt(&ok);
                if (!microCode->inRange(localEnumMnemonic, value)) {
                    errorString = "//ERROR: Value " + QString("%1").arg(value) + " is out of range for " + localIdentifier;
                    delete code;
                    return false;
                }
                microCode->set(localEnumMnemonic, value);
                state = Asm::PS_CONTINUE_PRE_SEMICOLON;
            }
            else {
                errorString = "//ERROR: Expected decimal number after " + localIdentifier + "=";
                delete code;
                return false;
            }
            break;

        case Asm::PS_CONTINUE_PRE_SEMICOLON:
            if (token == Asm::LT_COMMA) {
                state = Asm::PS_CONTINUE_PRE_SEMICOLON_POST_COMMA;
            }
            else if (token == Asm::LT_SEMICOLON) {
                state = Asm::PS_START_POST_SEMICOLON;
            }
            else if (token == Asm::LT_COMMENT) {
                microCode->cComment = tokenString;
                state = Asm::PS_COMMENT;
            }
            else if (token == Asm::LT_EMPTY) {
                state = Asm::PS_FINISH;
            }
            else {
                errorString = "//ERROR: Expected ',' or ';' after control signal";
                delete code;
                return false;
            }
            break;

        case Asm::PS_CONTINUE_PRE_SEMICOLON_POST_COMMA:
            if (token == Asm::LT_IDENTIFIER) {
                if (Pep::mnemonToDecControlMap.contains(tokenString.toUpper())) {
                    localEnumMnemonic = Pep::mnemonToDecControlMap.value(tokenString.toUpper());
                    if (microCode->has(localEnumMnemonic)) {
                        errorString = "//ERROR: Duplicate control signal, " + tokenString;
                        delete code;
                        return false;
                    }
                    localIdentifier = tokenString;
                    state = Asm::PS_EQUAL_DEC;
                }
                else if (Pep::mnemonToMemControlMap.contains(tokenString.toUpper())) {
                    localEnumMnemonic = Pep::mnemonToMemControlMap.value(tokenString.toUpper());
                    if (microCode->has(localEnumMnemonic)) {
                        errorString = "//ERROR: Duplicate control signal, " + tokenString;
                        delete code;
                        return false;
                    }
                    microCode->set(localEnumMnemonic, 1);
                    state = Asm::PS_CONTINUE_PRE_SEMICOLON;
                }
                else if (Pep::mnemonToClockControlMap.contains(tokenString.toUpper())) {
                    errorString = "//ERROR: Clock signal (" + tokenString + ") must appear after semicolon";
                    delete code;
                    return false;
                }
                else {
                    errorString = "//ERROR: Unrecognized control signal: " + tokenString;
                    delete code;
                    return false;
                }
            }
            else if (token == Asm::LT_SEMICOLON) {
                errorString = "//ERROR: Control signal expected after comma.";
                return false;
            }
            else if (token == Asm::LT_COMMENT) {
                microCode->cComment = tokenString;
                state = Asm::PS_COMMENT;
            }
            else if (token == Asm::LT_EMPTY) {
                state = Asm::PS_FINISH;
            }
            else {
                errorString = "//ERROR: Syntax error where control signal or comment expected";
                delete code;
                return false;
            }
            break;

        case Asm::PS_START_POST_SEMICOLON:
            if (token == Asm::LT_IDENTIFIER) {
                if (Pep::mnemonToClockControlMap.contains(tokenString.toUpper())) {
                    localEnumMnemonic = Pep::mnemonToClockControlMap.value(tokenString.toUpper());
                    if (microCode->has(localEnumMnemonic)) {
                        errorString = "//ERROR: Duplicate clock signal, " + tokenString;
                        delete code;
                        return false;
                    }
                    microCode->set(localEnumMnemonic, 1);
                    state = Asm::PS_CONTINUE_POST_SEMICOLON;
                }
                else if (Pep::mnemonToDecControlMap.contains(tokenString.toUpper())) {
                    errorString = "//ERROR: Control signal " + tokenString + " after ';'";
                    delete code;
                    return false;
                }
                else if (Pep::mnemonToMemControlMap.contains(tokenString.toUpper())) {
                    errorString = "//ERROR: Memory control signal " + tokenString + " after ';'";
                    delete code;
                    return false;
                }
                else {
                    errorString = "//ERROR: Unrecognized clock signal: " + tokenString;
                    delete code;
                    return false;
                }
            }
            else if (token == Asm::LT_SEMICOLON) {
                errorString = "//ERROR: Multiple semicolons.";
                delete code;
                return false;
            }
            else if (token == Asm::LT_COMMENT) {
                microCode->cComment = tokenString;
                state = Asm::PS_COMMENT;
            }
            else if (token == Asm::LT_EMPTY) {
                state = Asm::PS_FINISH;
            }
            else {
                errorString = "//ERROR: Syntax error where clock signal or comment expected.";
                delete code;
                return false;
            }
            break;

        case Asm::PS_CONTINUE_POST_SEMICOLON:
            if (token == Asm::LT_COMMA) {
                state = Asm::PS_START_POST_SEMICOLON;
            }
            else if (token == Asm::LT_SEMICOLON) {
                errorString = "//ERROR: Multiple semcolons ';'";
                delete code;
                return false;
            }
            else if (token == Asm::LT_COMMENT) {
                microCode->cComment = tokenString;
                state = Asm::PS_COMMENT;
            }
            else if (token == Asm::LT_EMPTY) {
                state = Asm::PS_FINISH;
            }
            else {
                errorString = "//ERROR: Expected ',' after clock signal";
                delete code;
                return false;
            }
            break;

        case Asm::PS_START_SPECIFICATION:
            if (token == Asm::LT_IDENTIFIER) {
                if (Pep::mnemonToMemSpecMap.contains(tokenString.toUpper())) {
                    localEnumMnemonic = Pep::mnemonToMemSpecMap.value(tokenString.toUpper());
                    state = Asm::PS_EXPECT_LEFT_BRACKET;
                }
                else if (Pep::mnemonToRegSpecMap.contains(tokenString.toUpper())) {
                    localEnumMnemonic = Pep::mnemonToRegSpecMap.value(tokenString.toUpper());
                    state = Asm::PS_EXPECT_REG_EQUALS;
                }
                else if (Pep::mnemonToStatusSpecMap.contains(tokenString.toUpper())) {
                    localEnumMnemonic = Pep::mnemonToStatusSpecMap.value(tokenString.toUpper());
                    state = Asm::PS_EXPECT_STATUS_EQUALS;
                }
                else {
                    errorString = "//ERROR: Unrecognized specification symbol: " + tokenString;
                    delete code;
                    return false;
                }
            }
            else {
                errorString = "//ERROR: Syntax error in specification.";
                delete code;
                return false;
            }
            break;

        case Asm::PS_EXPECT_LEFT_BRACKET:
            if (token == Asm::LT_LEFT_BRACKET) {
                state = Asm::PS_EXPECT_MEM_ADDRESS;
            }
            else {
                errorString = "//ERROR: Expected [ after Mem.";
                delete code;
                return false;
            }
            break;

        case Asm::PS_EXPECT_MEM_ADDRESS:
            if (token == Asm::LT_HEX_CONSTANT) {
                state = Asm::PS_EXPECT_RIGHT_BRACKET;
            }
            else {
                errorString = "//ERROR: Expected hex memory address after [.";
                delete code;
                return false;
            }
            break;

        case Asm::PS_EXPECT_RIGHT_BRACKET:
            if (token == Asm::LT_RIGHT_BRACKET) {
                state = Asm::PS_EXPECT_MEM_EQUALS;
            }
            else {
                errorString = "//ERROR: Expected ] after memory address.";
                delete code;
                return false;
            }
            break;

        case Asm::PS_EXPECT_MEM_EQUALS:
            if (token == Asm::LT_EQUALS) {
                state = Asm::PS_EXPECT_MEM_VALUE;
            }
            else {
                errorString = "//ERROR: Expected = after ].";
                delete code;
                return false;
            }
            break;

        case Asm::PS_EXPECT_MEM_VALUE:
            if (token == Asm::LT_HEX_CONSTANT) {
                state = Asm::PS_EXPECT_SPEC_COMMA;
            }
            else {
                errorString = "//ERROR: Expected hex constant after =.";
                delete code;
                return false;
            }
            break;

        case Asm::PS_EXPECT_SPEC_COMMA:
            if (token == Asm::LT_HEX_CONSTANT) {
                state = Asm::PS_EXPECT_SPEC_COMMA;
            }
            else {
                errorString = "//ERROR: Expected hex constant after =.";
                delete code;
                return false;
            }
            break;

        case Asm::PS_EXPECT_REG_EQUALS:
            break;

        case Asm::PS_EXPECT_REG_VALUE:
            break;

        case Asm::PS_EXPECT_STATUS_EQUALS:
            break;

        case Asm::PS_EXPECT_STATUS_VALUE:
            break;

        case Asm::PS_CONTINUE_SPECIFICATION:
            break;

        case Asm::PS_COMMENT:
            if (token == Asm::LT_EMPTY) {
                state = Asm::PS_FINISH;
            }
            else {
                // This error should not occur, as all characters are allowed in comments.
                errorString = "//ERROR: Problem detected after comment.";
                delete code;
                return false;
            }
            break;

        default:
            break;
        }
    }
    while (state != Asm::PS_FINISH);
    return true;
}

bool Asm::startsWithHexPrefix(QString str)
{
    if (str.length() < 2) return false;
    if (str[0] != '0') return false;
    if (str[1] == 'x' || str[1] == 'X') return true;
    return false;
}
