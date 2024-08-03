#include<bits/stdc++.h>
using namespace std;

// 单词
struct Token {
    string type; // "keyword", "identifier", "constant", "operator", "separator"
    string value;
};

int cnt = 0; //变量数
unordered_map<string, int> offset; //变量对应偏置值
tuple<int, string> info, info2;
unordered_map<string, vector<Token>> argsMap;
int len; // 参数/表达式长度
int sig;
bool funCall = false;
// string temp;

class SymbolTable {
public:
    // 进入一个函数的符号表作用域
    void enterFunction(const string& functionName) {
        currentFunction = functionName;
        functionTables[functionName] = SymbolMap();
        //cout << "enterFunction: " << functionName << endl;
    }

    // 退出函数的符号表作用域
    void exitFunction() {
        //cout << "exitFunction: " << currentFunction << endl;
        currentFunction.clear();
    }

    // 向当前函数的符号表添加符号
    void addSymbol(const string& name, int offset, const string& type) {
        if (!currentFunction.empty()) {
            functionTables[currentFunction][name] = {offset, type};
            // cout << "add: token = " << name << ", offset = " << offset << ", type = " << type << endl;
        }
    }

    // 根据符号名称查找其偏移量和类型，并返回二元组
    tuple<int, string> findSymbolInfo(const string& name) {
        if (!currentFunction.empty()) {
            SymbolMap& symMap = functionTables[currentFunction];
            if (symMap.find(name) != symMap.end()) {
                return make_tuple(symMap[name].first, symMap[name].second);
            }
        }
        // 默认返回 (-1, "") 表示未找到
        return make_tuple(-1, "");
    }

    // 打印当前函数的符号表
    void printCurrentSymbolTable() {
        if (!currentFunction.empty()) {
            cout << "currentFunction: " << currentFunction << " symbol table:" << endl;
            SymbolMap& symMap = functionTables[currentFunction];
            for (auto it = symMap.begin(); it != symMap.end(); ++it) {
                cout << "token: " << it->first << ", offset: " << it->second.first << ", type: " << it->second.second << endl;
            }
        } else {
            cout << "no active function" << endl;
        }
    }

private:
    // 符号映射表
    using SymbolMap = unordered_map<string, pair<int, string>>;
    // 每个函数的符号表
    unordered_map<string, SymbolMap> functionTables;
    // 当前处理的函数名
    string currentFunction;
};
SymbolTable symTable;

vector<Token> lexicalAnalysis(string& inputCode); // 词法分析
int precedence(const Token& op); // 运算符优先级
bool isOperator(const Token& token); // 是否为运算符
bool isOperand(const Token& token); // 是否为运算数
vector<Token> processMinus(vector<Token>& expr); // 处理一元运算符-
vector<Token> infixToPostfix(const vector<Token>& infix); // 中缀转后缀
void defFunc(vector<Token>& tokens);
void generateCode(vector<Token>& tokens); // 生成代码
pair<vector<Token>, vector<vector<Token>>> mergeFunc(vector<Token> expr);
void FunctionCallAsm(Token func, vector<Token> args);
void calExpression(vector<Token> expr); // 调用，生成汇编


int main(int argc, char* argv[]) {
    //ifstream sourceFile(argv[1]);
    ifstream sourceFile("D:\\desktop related\\current\\compiler\\1.txt");

    if (!sourceFile.is_open()) {
        cerr << "无法打开源文件。" << endl;
        return 1;
    }
    // 读取文件内容存入字符串inputCode中
    stringstream buffer;
    buffer << sourceFile.rdbuf();
    string inputCode = buffer.str();

    // 关闭文件
    sourceFile.close();

    //offset[temp] = 100;

    //开始
    cout << ".intel_syntax noprefix" << endl;
    //cout << ".global main" << endl;
    cout << ".extern printf" << endl;
    cout << ".data" << endl;
    cout << "format_str:" << endl;
    cout << " .asciz \"%d\\n\" " << endl;
    cout << ".text" << endl;

    vector<Token> ts = lexicalAnalysis(inputCode);
    defFunc(ts);
    //generateCode(ts);

//    cout << "leave" << endl;
//    cout << "ret" << endl;

    return 0;
}

vector<Token> lexicalAnalysis(string& inputCode) {
    vector<Token> tokens;

    // 定义用于匹配各种词法单元的正则表达式
    regex tokenPatterns(
            "\\b(println_int|int|return|main|void)\\b"   // 匹配关键字
            "|([a-zA-Z_][a-zA-Z0-9_]*)"             // 匹配标识符
            "|(\\d+)"                               // 匹配数字常量
            "|(==|!=|<=|>=|&&|\\|\\||[-=!~+*/%<>&|^])" // 匹配操作符
            "|([;]|[()]|[{}]|[,])"                      // 分隔符
    );

    // 使用正则迭代器匹配整个字符串中的词法单元
    auto words_begin = sregex_iterator(inputCode.begin(), inputCode.end(), tokenPatterns);
    auto words_end = sregex_iterator();

    for (sregex_iterator i = words_begin; i != words_end; ++i) {
        smatch match = *i;
        string token = match.str();

        // 决定Token类型
        if (match[1].matched) tokens.push_back({"keyword", token});
        else if (match[2].matched) tokens.push_back({"identifier", token});
        else if (match[3].matched) tokens.push_back({"constant", token});
        else if (match[4].matched) tokens.push_back({"operator", token});
        else if (match[5].matched) tokens.push_back({"separator", token});
    }

    return tokens;
}

int precedence(const Token& op) {
    if (op.value == "||") return 1;
    if (op.value == "&&") return 2;
    if (op.value == "|") return 3;
    if (op.value == "^") return 4;
    if (op.value == "&") return 5;
    if (op.value == "==" || op.value == "!=") return 6;
    if (op.value == "<" || op.value == "<=" || op.value == ">" || op.value == ">=") return 7;
    if (op.value == "+" || op.value == "-") return 8;
    if (op.value == "*" || op.value == "/" || op.value == "%") return 9;
    if (op.value == "!" || op.value == "~") return 10; // 一元运算符
    //if (op.value == "!" || op.value == "~" || op.value == "-") return 10; // 一元运算符
    return 0;
}

bool isOperator(const Token& token) {
    return token.type == "operator";
}

bool isOperand(const Token& token) {
    return token.type == "identifier" || token.type == "constant" || token.type == "function";
}

vector<Token> processMinus(vector<Token>& expr) {
    vector<Token> processed;
    bool insert = false;
    Token zeroToken = {"constant", "0"};
    Token leftToken = {"separator", "("};
    Token rightToken = {"separator", ")"};

    if (!expr.empty()) {
        for (size_t i = 0; i < expr.size(); ++i) {
            const Token& current = expr[i];
            // 如果当前Token是减号，并且是表达式的第一个Token或前一个Token不是数字
            if (current.value == "-" && (i == 0 || (expr[i - 1].type != "constant" && expr[i - 1].type != "identifier"))) {
                // 插入一个0之前，确保不在")"后
                if (i == 0 || expr[i - 1].value != ")") {
                    processed.push_back(leftToken);
                    processed.push_back(zeroToken);
                    insert = true;
                }
            }
            // 添加当前Token到处理后的列表
            processed.push_back(current);
            if(insert){
                i++;
                processed.push_back(expr[i]);
                processed.push_back(rightToken);
                insert = false;
            }
        }
    }

    return processed;
}

vector<Token> infixToPostfix(const vector<Token>& infix) {
    vector<Token> postfix;
    stack<Token> ops;

    for (const Token& token : infix) {
        if (isOperand(token)) {
            postfix.push_back(token);
        } else if (token.type == "separator" && token.value == "(") {
            ops.push(token);
        } else if (token.type == "separator" && token.value == ")") {
            while (!ops.empty() && ops.top().value != "(") {
                postfix.push_back(ops.top());
                ops.pop();
            }
            ops.pop();
        } else if (isOperator(token)) {
            while (!ops.empty() && precedence(ops.top()) >= precedence(token)) {
                postfix.push_back(ops.top());
                ops.pop();
            }
            ops.push(token);
        }
    }

    while (!ops.empty()) {
        postfix.push_back(ops.top());
        ops.pop();
    }

    return postfix;
}

void defFunc(vector<Token>& tokens){
    for (size_t i = 0; i < tokens.size(); ++i) {
        // 处理函数定义 void f(...   int f(...
//        if (tokens[i].type == "keyword" && (tokens[i].value == "void" || tokens[i].value == "int")
//            && (tokens[i + 1].type == "identifier" || (tokens[i + 1].type == "keyword" && tokens[i].value == "main"))
//            && tokens[i + 2].type == "separator" && tokens[i + 2].value == "(") {
            string funcName = tokens[i + 1].value;
            cout << ".global " << funcName << endl;
            cout << funcName << ":" << endl;
            cout << "push ebp" << endl;
            cout << "mov ebp, esp" << endl;
            cout << "sub esp, 0xffff" << endl;

            symTable.enterFunction(funcName);

            // 处理函数参数
            i += 3;
            int arg_offset = 8;
            while (tokens[i].type != "separator" || tokens[i].value != ")") {
                if (tokens[i].type == "keyword" && tokens[i].value == "int"
                    && tokens[i + 1].type == "identifier") {
                    offset[tokens[i + 1].value] = arg_offset;
                    symTable.addSymbol(tokens[i + 1].value, arg_offset, "parameter");
                    //cout << "mov DWORD PTR [ebp+" << offset[tokens[i + 1].value] << "], 0 " << " # int " << tokens[i + 1].value << endl;
                    arg_offset += 4;
                    i += 2;
                } else {
                    ++i;
                }
            }
            i++;
            // 处理自定义函数内的语句
            vector<Token> func;
            if (tokens[i].type == "separator" && tokens[i].value == "{") {
                i++;
                while (tokens[i].type != "separator" || tokens[i].value != "}") {
                    func.push_back(tokens[i]);
                    i++;
                }
//                for (const auto& token : func) {
//                    cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//                }
                generateCode(func);

                cout << "leave" << endl;
                cout << "ret" << endl;
                // symTable.printCurrentSymbolTable();
                symTable.exitFunction();
            }
        }
//    }
}

void generateCode(vector<Token>& tokens) {
//    for (const auto& token : tokens) {
//        cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//    }
    // 从头到尾依次处理
    for (size_t i = 0; i < tokens.size(); ++i) {
        // 处理单独的函数调用 f(...);
        if (tokens[i].type == "identifier"
                 && tokens[i + 1].type == "separator" && tokens[i + 1].value == "(") {
            string funcName = tokens[i].value;
            len = 0;
            i += 2;
            // 例如 f(a,a+b);
            while(!(tokens[i].type == "separator" && tokens[i].value == ")")){
                len++;
                i++;
            }
            i -= len;

            vector<Token> exp;
            exp.clear();
            while(len > 0){
                exp.push_back(tokens[i]);
                i++;
                len--;
            }
//            calExpression(exp);

//            for (const auto& token : exp) {
//                cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//            }

            int leftNum = 0, rightNum = 0, argNum = 0; //左右括号的数量、参数个数
            vector<Token> expr;
            vector<vector<Token>> exprList;
            // 确保 exprList 有足够的大小
            size_t argsNum = 10;
            if (exprList.size() <= argsNum) {
                exprList.resize(argsNum + 1); // 调整大小
            }
            for (size_t i = 0; i < exp.size(); i++){
                while(leftNum != rightNum || exp[i].value != ","){
                    if(exp[i].value == "("){
                        leftNum++;
                    }else if(exp[i].value == ")"){
                        rightNum++;
                    }
                    expr.push_back(exp[i]);
                    i++;
                    if(i >= exp.size()){
                        break;
                    }
                }
                if (!expr.empty()) {
//                    cout << "expr: " << endl;
//                    for (const auto& token : expr) {
//                        cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//                    }
                    // calExpression(expr);
                    exprList.push_back(expr);
                    argNum++;
                    expr.clear();
                }
            }

            reverse(exprList.begin(), exprList.end());

            for (size_t i = 0; i < exprList.size(); ++i){
                calExpression(exprList[i]);
            }
//            for (const auto& token : args) {
//                cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//            }

            // 调用函数
            cout << "call " << funcName << endl;
            cout << "add esp, " << argNum * 4 << endl; // 清理参数
        }
        // 处理变量声明 int x...
        else if (tokens[i].type == "keyword" && tokens[i].value == "int") {
            i++;
            while (tokens[i].type == "identifier" || tokens[i].type == "separator") {
                if (tokens[i].type == "identifier") {
                    string varName = tokens[i].value;
                    //cout << varName <<endl;
                    i++;
                    int leftNum = 0, rightNum = 0;
                    // 声明时直接初始化 例如int x = ...
                    if (tokens[i].type == "operator" && tokens[i].value == "=" ) {
                        i++;
                        // int x = expression;
                        len = 0;// int c = a + b;
                        // 例如 int x, y = a + b, z;或 int x, z, y = a + b;中的y
                        while(leftNum != rightNum || tokens[i].type != "separator" || !(tokens[i].value == "," || tokens[i].value == ";")){
                            if(tokens[i].value == "("){
                                leftNum++;
                            }else if(tokens[i].value == ")"){
                                rightNum++;
                            }
                            len++;
                            i++;
                        }
                        i -= len;

                        offset[varName] = (++cnt) * 4;
                        symTable.addSymbol(varName, offset[varName], "local");

                        if(len < 2 && tokens[i].type == "identifier"){
                            info = symTable.findSymbolInfo(tokens[i].value);
                            if(get<1>(info) == "parameter"){
                                cout << "push DWORD PTR [ebp+" << offset[tokens[i].value] << "]"  << endl;
                                cout << "pop DWORD PTR [ebp-" << offset[varName] << "]"  << endl;
                                //cout << "mov DWORD PTR [ebp-" << offset[varName] << "], DWORD PTR [ebp+" << offset[tokens[i].value] << "]" <<"     # int " << varName << " = " << tokens[i].value << endl;
                            }else if(get<1>(info) == "local"){
                                cout << "push DWORD PTR [ebp-" << offset[tokens[i].value] << "]"  << endl;
                                cout << "pop DWORD PTR [ebp-" << offset[varName] << "]"  << endl;
                                //cout << "mov DWORD PTR [ebp-" << offset[varName] << "], DWORD PTR [ebp-" << offset[tokens[i].value] << "]" <<"     # int " << varName << " = " << tokens[i].value << endl;
                            }
                            i++;
                        }else if(len < 2 && tokens[i].type == "constant"){
                            int value = stoi(tokens[i].value);
//                            offset[varName] = (++cnt) * 4;
//                            symTable.addSymbol(varName, offset[varName], "local");
                            cout << "mov DWORD PTR [ebp-" << offset[varName] << "], " << value << "     # int " << varName << " = " << value << endl;
                            i++;
                        }else{
                            vector<Token> exp;
                            exp.clear();
                            while(len > 0){
                                exp.push_back(tokens[i]);
                                i++;
                                len--;
                            }
//                            for (const auto& token : exp) {
//                                cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//                            }
                            calExpression(exp);
                            cout << "pop eax" << endl;  // 将最终结果弹出到 eax
                            cout << "mov DWORD PTR [ebp-" << offset[varName] << "], eax" << endl;
                        }
                    // 不初始化
                    } else {
                        offset[varName] = (++cnt) * 4;
                        symTable.addSymbol(varName, offset[varName], "local");
                        cout << "mov DWORD PTR [ebp-" << offset[varName] << "], 0     # int " << varName << endl;
                    }
                }
                if (tokens[i].type == "separator" && tokens[i].value == ",") {
                    i++;
                } else if (tokens[i].type == "separator" && tokens[i].value == ";") {
                    break;
                }
            }
            continue;
        }
        // 处理return  return 0/x/f(.../expression
        else if (tokens[i].type == "keyword" && tokens[i].value == "return") {
            i++; // 跳过return
            len = 0;
            // 例如 return a+b;
            while(tokens[i].type != "separator" || tokens[i].value != ";"){
                len++;
                i++;
            }
            i -= len;// 重置i至(后一个token

            // return表达式
            if(len>=2){
                vector<Token> exp;
                exp.clear();
                while(len > 0){
                    exp.push_back(tokens[i]);
                    i++;
                    len--;
                }
                calExpression(exp);
                cout << "pop eax" << endl;  // 将最终结果弹出到 eax
            // return变量
            }else if (tokens[i].type == "identifier") {
                info = symTable.findSymbolInfo(tokens[i].value);
                if(get<1>(info) == "parameter"){
                    cout << "mov eax, DWORD PTR [ebp+" << get<0>(info) << "]      # return " << tokens[i].value << endl;
                }else if(get<1>(info) == "local"){
                    cout << "mov eax, DWORD PTR [ebp-" << get<0>(info) << "]      # return " << tokens[i].value << endl;
                }
            // return常量
            } else if (tokens[i].type == "constant") {
                cout << "mov eax, " << tokens[i].value << "      # return " << tokens[i].value << endl;
            }
            break;
        }
        // 处理println_int println_int(0/x/f(.../expression
        else if (tokens[i].type == "keyword" && tokens[i].value == "println_int") {
            i += 2; // 跳过println_int(
            len = 0;
            // 例如 println_int(a+b);
            while(tokens[i].type != "separator" || tokens[i].value != ";"){
                len++;
                i++;
            }
            i -= len;// 重置i至(后一个token
            len--; // len即括号内参数的长度

            // 参数为表达式
            if(len >= 2){
                vector<Token> exp;
                exp.clear();
                while(len>0){
                    exp.push_back(tokens[i]);
                    i++;
                    len--;
                }
                calExpression(exp);
//                if(funCall){
//                    cout << "push eax" << endl;
//                }
                cout << "push offset format_str" << endl;  // formatStr 应该包含 "%d\n"
                cout << "call printf" << endl;
                cout << "add esp, 8" << endl;  // 清理栈（两个参数：整数和格式字符串）
                i++;
                continue;
            // 参数为变量
            }else if (tokens[i].type == "identifier") {
                // 加载标识符对应的值到 eax （用于 printf）
//                cout << "mov eax, DWORD PTR [ebp-" << offset[tokens[i + 2].value] << "]" << endl;
//                cout << "push eax" << endl;  // 将 eax 推入栈中作为 printf 的参数
                info = symTable.findSymbolInfo(tokens[i].value);
                if(get<1>(info) == "parameter"){
                    //cout << tokens[i + 2].value << endl;
                    cout << "push DWORD PTR [ebp+" << get<0>(info) << "]" << endl;
                }else if(get<1>(info) == "local"){
                    cout << "push DWORD PTR [ebp-" << get<0>(info) << "]" << endl;
                }
            // 参数为常量
            } else if (tokens[i].type == "constant") {
                // 直接将常数值推入栈中作为 printf 的参数
                cout << "push " << tokens[i].value << endl;
            }
            // 设置 printf 的格式字符串，这里简化处理，假设总是整数
            cout << "push offset format_str" << endl;  // formatStr 应该包含 "%d\n"
            cout << "call printf" << endl;
            cout << "add esp, 8" << endl;  // 清理栈（两个参数：整数和格式字符串）

            // 更新循环索引，跳过已处理的 tokens
            i += 2;
            continue;
        }
        // 处理赋值语句 x=...
        else if (tokens[i].type == "identifier" && tokens[i + 1].type == "operator" && tokens[i + 1].value == "=") {
            vector<Token> s;
            int expressionLen = 0; // 表达式长度

            string var = tokens[i].value;
            info2 = symTable.findSymbolInfo(var); // 被赋值的变量信息
            i += 2; // 跳过标识符和赋值操作符
            while (i < tokens.size() && tokens[i].value != ";") {
                s.push_back(tokens[i]);
                ++expressionLen;
                ++i;
            }
//            for (const auto& token : s) {
//                cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//            }

            // 简单直接赋值 x=1/x=y
            if (expressionLen == 1) {
                Token rightToken = s[0];
                if (rightToken.type == "identifier") {
                    info = symTable.findSymbolInfo(rightToken.value);
                    if(get<1>(info) == "parameter"){
                        cout << "mov eax, DWORD PTR [ebp+" << get<0>(info) << "]" << endl;
                        if(get<1>(info2) == "parameter"){
                            cout << "mov DWORD PTR [ebp+" << offset[var] << "], eax    # " << var << " = " << rightToken.value << endl;
                        }else if(get<1>(info2) == "local"){
                            cout << "mov DWORD PTR [ebp-" << offset[var] << "], eax    # " << var << " = " << rightToken.value << endl;
                        }
                    }else if(get<1>(info) == "local"){
                        cout << "mov eax, DWORD PTR [ebp-" << get<0>(info) << "]" << endl;
                        if(get<1>(info2) == "parameter"){
                            cout << "mov DWORD PTR [ebp+" << offset[var] << "], eax    # " << var << " = " << rightToken.value << endl;
                        }else if(get<1>(info2) == "local"){
                            cout << "mov DWORD PTR [ebp-" << offset[var] << "], eax    # " << var << " = " << rightToken.value << endl;
                        }
                    }
                } else if (rightToken.type == "constant") {
                    // 右侧是一个常量，直接将值赋给左侧变量
                    cout << "mov eax, " << rightToken.value << "    # " << var
                         << " = " << rightToken.value << endl;
                    cout << "mov DWORD PTR [ebp-" << offset[var] << "], eax" << endl;
                }
            // x=expression
            } else {
                calExpression(s);
//                if(!funCall){
//                    cout << "pop eax" << endl;  // 将最终结果弹出到 eax
//                }
                cout << "pop eax" << endl;
                if(get<1>(info2) == "parameter"){
                    cout << "mov DWORD PTR [ebp+" << offset[var] << "], eax" << endl;
                }else if(get<1>(info2) == "local"){
                    cout << "mov DWORD PTR [ebp-" << offset[var] << "], eax" << endl;
                }
                s.clear();
            }
        }
    }
}

pair<vector<Token>, vector<vector<Token>>> mergeFunc(vector<Token> expr){
    int leftNum = 0, rightNum = 0, funDex=-1, sig = 0, argNum = 0;
    vector<Token> newExpr;
    vector<vector<Token>> args;
    // 确保 args 有足够的大小
    size_t argsNum = 10;
    if (args.size() <= argsNum) {
        args.resize(argsNum + 1); // 调整大小以包含 argNum 索引
    }
    for (size_t i = 0; i < expr.size(); ++i) {
        if(i + 1 < expr.size() && (expr[i].type == "identifier" && expr[i + 1].type == "separator" && expr[i + 1].value == "(")){
            funDex = i;
            expr[i].type = "function";
//            cout << "function name: " << expr[i].value << ". type: " << expr[i].type << endl;
            leftNum++;
            i += 2;
            while(leftNum != rightNum){
                if(expr[i].value == "("){
                    leftNum++;
                }else if(expr[i].value == ")"){
                    rightNum++;
                }
                if(leftNum != rightNum){
                    args[argNum].push_back(expr[i]);
                    i++;
                }
            }
            if(i + 1 >= expr.size()){
                sig = 1;
            }else{
                i++;
            }
        }
        if(funDex != -1){
            newExpr.push_back(expr[funDex]);
            argNum++;
            funDex = -1;
        }
        if(sig == 1){
            break;
        }
        newExpr.push_back(expr[i]);
    }
    return make_pair(newExpr, args);
}

void FunctionCallAsm(Token func, vector<Token> args) {
    string funcName = func.value;
//    cout << "calling function:" << funcName << endl;
//    cout << "args: " << endl;
//    for (const auto& token : args) {
//        cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//    }
    int leftNum = 0, rightNum = 0, argNum = 0; //左右括号的数量
    vector<Token> expr;
    vector<vector<Token>> exprList;
    // 确保 exprList 有足够的大小
    size_t argsNum = 10;
    if (exprList.size() <= argsNum) {
        exprList.resize(argsNum + 1); // 调整大小
    }
    for (size_t i = 0; i < args.size(); i++){
        while(leftNum != rightNum || args[i].value != ","){
            if(args[i].value == "("){
                leftNum++;
            }else if(args[i].value == ")"){
                rightNum++;
            }
            expr.push_back(args[i]);
            i++;
            if(i >= args.size()){
                break;
            }
        }
        if (!expr.empty()) {
//            cout << "expr: " << endl;
//            for (const auto& token : expr) {
//                cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//            }
            // calExpression(expr);
            exprList.push_back(expr);
            argNum++;
            expr.clear();
        }
    }

    reverse(exprList.begin(), exprList.end());

    for (size_t i = 0; i < exprList.size(); ++i){
        calExpression(exprList[i]);
    }

    // 调用函数
    cout << "call " << funcName << endl;
    cout << "add esp, " << argNum * 4 << endl; // 清理参数
}

void calExpression(vector<Token> expression){
//    vector<Token> postfix = infixToPostfix(expression);
    vector<Token> postfix;
    vector<vector<Token>> args;
    int argNum = 0;
    postfix = processMinus(expression);

    auto res = mergeFunc(postfix);
    postfix = res.first;
    args = res.second;
//    for (const auto& token : postfix) {
//        cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//    }
    postfix = infixToPostfix(postfix);

//    for (const auto& token : postfix) {
//        cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//    }

    for (size_t i = 0; i < postfix.size(); ++i) {
        if (postfix[i].type == "identifier") {
            info = symTable.findSymbolInfo(postfix[i].value);
            if(get<1>(info) == "parameter"){
                cout << "push DWORD PTR [ebp+" << get<0>(info) << "]" << endl;
            }else if(get<1>(info) == "local"){
                cout << "push DWORD PTR [ebp-" << get<0>(info) << "]" << endl;
            }
        } else if (postfix[i].type == "constant") {
            cout << "push " << postfix[i].value << endl;
        }else if(postfix[i].type == "function"){
//            cout << "functionCall" << endl;
//            for (const auto& token : args[argNum]) {
//                cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//            }
            FunctionCallAsm(postfix[i], args[argNum]);
            cout << "push eax" << endl;
            argNum++;
        } else if (isOperator(postfix[i]) &&
        (postfix[i].value == "+" || postfix[i].value == "-" || postfix[i].value == "*" ||postfix[i].value == "/" ||
                postfix[i].value == "%" ||postfix[i].value == "&" ||postfix[i].value == "|" ||postfix[i].value == "^" ||
                postfix[i].value == "<" ||postfix[i].value == "<=" ||postfix[i].value == ">" ||postfix[i].value == ">=" ||
                postfix[i].value == "==" ||postfix[i].value == "!=" ||postfix[i].value == "&&"||postfix[i].value == "||")) {
            if(funCall){
                cout << "pop ebx" << endl;  // 弹出第二个操作数到 ebx
            }else{
                cout << "pop ebx" << endl;  // 弹出第二个操作数到 ebx
                cout << "pop eax" << endl;  // 弹出第一个操作数到 eax
            }

            if (postfix[i].value == "+") {
                cout << "add eax, ebx" << endl;
            } else if (postfix[i].value == "-") {
                cout << "sub eax, ebx" << endl;
            } else if (postfix[i].value == "*") {
                cout << "imul eax, ebx" << endl;
            } else if (postfix[i].value == "/") {
                cout << "cdq" << endl;  // 扩展 eax 到 edx:eax
                cout << "idiv ebx" << endl;
            } else if (postfix[i].value == "%") {
                cout << "cdq" << endl;
                cout << "idiv ebx" << endl;
                cout << "mov eax, edx" << endl;
            } else if (postfix[i].value == "&") {
                cout << "and eax, ebx" << endl;
            } else if (postfix[i].value == "|") {
                cout << "or eax, ebx" << endl;
            } else if (postfix[i].value == "^") {
                cout << "xor eax, ebx" << endl;
            } else if (postfix[i].value == "<") {
                cout << "cmp eax, ebx" << endl;
                cout << "setl al" << endl;
                cout << "movzx eax, al" << endl;
            } else if (postfix[i].value == "<=") {
                cout << "cmp eax, ebx" << endl;
                cout << "setle al" << endl;
                cout << "movzx eax, al" << endl;
            } else if (postfix[i].value == ">") {
                cout << "cmp eax, ebx" << endl;
                cout << "setg al" << endl;
                cout << "movzx eax, al" << endl;
            } else if (postfix[i].value == ">=") {
                cout << "cmp eax, ebx" << endl;
                cout << "setge al" << endl;
                cout << "movzx eax, al" << endl;
            } else if (postfix[i].value == "==") {
                cout << "cmp eax, ebx" << endl;
                cout << "sete al" << endl;
                cout << "movzx eax, al" << endl;
            } else if (postfix[i].value == "!=") {
                cout << "cmp eax, ebx" << endl;
                cout << "setne al" << endl;
                cout << "movzx eax, al" << endl;
            } else if (postfix[i].value == "&&"){
                cout << "test eax, eax" << endl;
                cout << "jz false" << sig << endl;
                cout << "test ebx, ebx" << endl;
                cout << "jz false" << sig+1 << endl;
                cout << "mov eax, 1" << endl;
                cout << "jmp done" << sig << endl;
                cout << "false" << sig << ":" << endl;
                cout << "false" << sig+1 << ":" << endl;
                cout << "mov eax, 0" << endl;
                cout << "done" << sig << ":" << endl;
                sig += 2;
            } else if (postfix[i].value == "||"){
                cout << "test eax, eax" << endl;
                cout << "jnz true" << sig << endl;
                cout << "test ebx, ebx" << endl;
                cout << "jnz true" << sig+1 << endl;
                cout << "mov eax, 0" << endl;
                cout << "jmp done" << sig << endl;
                cout << "true" << sig << ":" << endl;
                cout << "true" << sig+1 << ":" << endl;
                cout << "mov eax, 1" << endl;
                cout << "done" << sig << ":" << endl;
                sig += 2;
            }

            cout << "push eax" << endl;
        } else if (isOperator(postfix[i]) && (postfix[i].value == "!" || postfix[i].value == "~")) {
            if(!funCall){
                cout << "pop eax" << endl;
            }
            if (postfix[i].value == "!") {
                cout << "test eax, eax" << endl;
                cout << "setz al" << endl;
                cout << "movzx eax, al" << endl;
            } else if (postfix[i].value == "~") {
                cout << "not eax" << endl;
            }

            cout << "push eax" << endl;
        }
    }
//    cout << "pop eax" << endl;  // 将最终结果弹出到 eax
}
