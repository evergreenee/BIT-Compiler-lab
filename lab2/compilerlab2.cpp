#include<bits/stdc++.h>
using namespace std;

// 单词
struct Token {
    string type; // "keyword", "identifier", "constant", "operator", "separator"
    string value;
};

vector<Token> lexicalAnalysis(const string& inputCode); // 词法分析
int precedence(const Token& op); // 运算符优先级
bool isOperator(const Token& token); // 是否为运算符
bool isOperand(const Token& token); // 是否为运算数
vector<Token> process(const vector<Token>& infix); // 处理负数
vector<Token> infixToPostfix(const vector<Token>& infix); // 中缀转后缀
void generateCode(const vector<Token>& tokens); // 生成代码

int main(int argc, char* argv[]) {
    ifstream sourceFile(argv[1]);
    //ifstream sourceFile("C:\\Users\\Lenovo\\Desktop\\compiler\\1.txt");

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

    vector<Token> ts = lexicalAnalysis(inputCode);
    generateCode(ts);

    cout << "leave" << endl;
    cout << "ret" << endl;

    return 0;
}

vector<Token> lexicalAnalysis(const string& inputCode) {
    vector<Token> tokens;

    // 定义用于匹配各种词法单元的正则表达式
    regex tokenPatterns(
            "\\b(println_int|int|return|main)\\b"   // 匹配关键字
            "|([a-zA-Z_][a-zA-Z0-9_]*)"             // 匹配标识符
            "|(\\d+)"                               // 匹配数字常量
            "|(==|!=|<=|>=|[-=\\+\\*/%<>\\|&^])" // 匹配操作符和
            "|([;]|[()]|[{}])"// 分隔符
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
    if (op.value == "|") return 1;
    if (op.value == "^") return 2;
    if (op.value == "&") return 3;
    if (op.value == "==" || op.value == "!=") return 4;
    if (op.value == "<" || op.value == "<=" || op.value == ">" || op.value == ">=") return 5;
    if (op.value == "+" || op.value == "-") return 6;
    if (op.value == "*" || op.value == "/" || op.value == "%") return 7;
    return 0;
}

bool isOperator(const Token& token) {
    return token.type == "operator";
}

bool isOperand(const Token& token) {
    return token.type == "identifier" || token.type == "constant";
}

vector<Token> process(const vector<Token>& infix) {
    vector<Token> processed;
    Token zeroToken = {"constant", "0"};

    if (!infix.empty()) {
        for (size_t i = 0; i < infix.size(); ++i) {
            const Token& current = infix[i];
            // 如果当前Token是减号，并且是表达式的第一个Token或前一个Token不是数字
            if (current.value == "-" && (i == 0 || (infix[i - 1].type != "constant" && infix[i - 1].type != "identifier"))) {
                // 插入一个0之前，确保不在")"后
                if (i == 0 || infix[i - 1].value != ")") {
                    processed.push_back(zeroToken);
                }
            }

            // 添加当前Token到处理后的列表
            processed.push_back(current);
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

void generateCode(const vector<Token>& tokens) {
//    for (const auto& token : tokens) {
//        cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//    }
    int cnt = 0; //变量数
    unordered_map<string, int> offset; //变量对应偏置值
    int argv = 0;

    //开始
    cout << ".intel_syntax noprefix" << endl;
    cout << ".global main" << endl;
    cout << ".extern printf" << endl;
    cout << ".data" << endl;
    cout << "format_str:" << endl;
    cout << " .asciz \"%d\\n\" " << endl;
    cout << ".text" << endl;

    // 从头到尾依次处理
    for (size_t i = 0; i < tokens.size(); ++i) {
        // 处理main函数
        // 无参数：int main()
        if (tokens[i].type == "keyword" && tokens[i].value == "int"
            && tokens[i + 1].type == "keyword" && tokens[i + 1].value == "main"
            && tokens[i + 2].type == "separator" && tokens[i + 2].value == "("
            && tokens[i + 3].type == "separator" && tokens[i + 3].value == ")") {
            cout << "main:" << endl;
            cout << "push ebp" << endl;               // 保存旧的基指针
            cout << "mov ebp, esp" << endl;           // 新的基指针设置为栈顶
            cout << "sub esp, 0x100" << endl;         // 为局部变量分配栈空间
            i += 4;
            continue;
            // 带参数：int main(int argc, int argv),只判断到"("后的"int"即可
        } else if (tokens[i].type == "keyword" && tokens[i].value == "int"
                   && tokens[i + 1].type == "keyword" && tokens[i + 1].value == "main"
                   && tokens[i + 2].type == "separator" && tokens[i + 2].value == "("
                   && tokens[i + 3].type == "keyword" && tokens[i + 3].value == "int") {
            cout << "main:" << endl;
            cout << "push ebp" << endl;               // 保存旧的基指针
            cout << "mov ebp, esp" << endl;           // 新的基指针设置为栈顶
            cout << "sub esp, 0x100" << endl;         // 为局部变量分配栈空间
            i += 8;
            argv = 1;
            continue;
        }
            // 处理变量声明
        else if (tokens[i].type == "keyword" && tokens[i].value == "int") {
            offset[tokens[i+1].value] = (++cnt) * 4;
            cout << "mov DWORD PTR [ebp-" << offset[tokens[i + 1].value] << "], 0     # int " << tokens[i + 1].value
                 << endl;
            i += 2;
        }
            // 处理return
        else if (tokens[i].type == "keyword" && tokens[i].value == "return") {
//            cout << "mov eax, DWORD PTR [ebp-" << offset[tokens[1].value] << "]      # return " << tokens[1].value
//                 << endl;
            if (tokens[i + 1].type == "identifier") {
                // 确保标识符在 offset 映射中存在
                if (offset.find(tokens[i + 1].value) != offset.end()) {
                    cout << "mov eax, DWORD PTR [ebp-" << offset[tokens[i + 1].value] << "]      # return " << tokens[i + 1].value
                         << endl;
                } else {
                    cout << "Error: Identifier '" << tokens[i + 1].value << "' not found." << endl;
                }
            } else if (tokens[i + 1].type == "constant") {
                cout << "mov eax, " << tokens[i + 1].value << "      # return " << tokens[i + 1].value << endl;
            }
            break;
        }
            // 处理println_int
        else if (tokens[i].type == "keyword" && tokens[i].value == "println_int") {
            if (tokens[i + 2].type == "identifier") {
                // 加载标识符对应的值到 eax （用于 printf）
//                cout << "mov eax, DWORD PTR [ebp-" << offset[tokens[i + 2].value] << "]" << endl;
//                cout << "push eax" << endl;  // 将 eax 推入栈中作为 printf 的参数
                cout << "push DWORD PTR [ebp-" << offset[tokens[i + 2].value] << "]" << endl;
            } else if (tokens[i + 2].type == "constant") {
                // 直接将常数值推入栈中作为 printf 的参数
                cout << "push " << tokens[i + 2].value << endl;
            }
            // 设置 printf 的格式字符串，这里简化处理，假设总是整数
            cout << "push offset format_str" << endl;  // formatStr 应该包含 "%d\n"
            cout << "call printf" << endl;
            cout << "add esp, 8" << endl;  // 清理栈（两个参数：整数和格式字符串）

            // 更新循环索引，跳过已处理的 tokens
            i += 4;
            continue;
        }
            // 处理赋值语句
        else if (tokens[i].type == "identifier" && tokens[i + 1].type == "operator" && tokens[i + 1].value == "=") {
            vector<Token> s;
            int expressionLen = 0; // 表达式长度

            string var = tokens[i].value;
            i += 2; // 跳过标识符和赋值操作符
            while (i < tokens.size() && tokens[i].value != ";") {
                s.push_back(tokens[i]);
                ++expressionLen;
                ++i;
            }

            // 简单直接赋值
            if (expressionLen == 1) {
                Token rightToken = s[0];
                if (rightToken.type == "identifier") {
                    // 右侧是一个变量，生成代码从这个变量读取值并存到左侧变量
                    cout << "mov eax, DWORD PTR [ebp-" << offset[rightToken.value] << "]" << endl;
                    cout << "mov DWORD PTR [ebp-" << offset[var] << "], eax    # " << var << " = " << rightToken.value << endl;
                } else if (rightToken.type == "constant") {
                    // 右侧是一个常量，直接将值赋给左侧变量
                    cout << "mov eax, " << rightToken.value << "    # " << var
                         << " = " << rightToken.value << endl;
                    cout << "mov DWORD PTR [ebp-" << offset[var] << "], eax" << endl;
//                    cout << "mov DWORD PTR [ebp-" << offset[var] << "], " << rightToken.value << "    # " << var
//                         << " = " << rightToken.value << endl;
                }
            } else {
                vector<Token> processed = process(s);
                vector<Token> postfix = infixToPostfix(processed);
//                vector<Token> postfix = infixToPostfix(s);

//                for (const Token &token : postfix) {
//                    cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//                }

                for (const Token &token: postfix) {
                    if (token.type == "identifier") {
                        cout << "push DWORD PTR [ebp-" << offset[token.value] << "]" << endl;
                    } else if (token.type == "constant") {
                        cout << "push " << token.value << endl;
                    } else if (isOperator(token)) {
                        cout << "pop ebx" << endl;  // 弹出第二个操作数到 ebx
                        cout << "pop eax" << endl;  // 弹出第一个操作数到 eax

                        if (token.value == "+") {
                            cout << "add eax, ebx" << endl;
                        } else if (token.value == "-") {
                            cout << "sub eax, ebx" << endl;
                        } else if (token.value == "*") {
                            cout << "imul eax, ebx" << endl;
                        } else if (token.value == "/") {
                            cout << "cdq" << endl;  // 扩展 eax 到 edx:eax
                            cout << "idiv ebx" << endl;
                        } else if (token.value == "%") {
                            cout << "cdq" << endl;
                            cout << "idiv ebx" << endl;
                            cout << "mov eax, edx" << endl;
                        } else if (token.value == "&") {
                            cout << "and eax, ebx" << endl;
                        } else if (token.value == "|") {
                            cout << "or eax, ebx" << endl;
                        } else if (token.value == "^") {
                            cout << "xor eax, ebx" << endl;
                        } else if (token.value == "<") {
                            cout << "cmp eax, ebx" << endl;
                            cout << "setl al" << endl;
                            cout << "movzx eax, al" << endl;
                        } else if (token.value == "<=") {
                            cout << "cmp eax, ebx" << endl;
                            cout << "setle al" << endl;
                            cout << "movzx eax, al" << endl;
                        } else if (token.value == ">") {
                            cout << "cmp eax, ebx" << endl;
                            cout << "setg al" << endl;
                            cout << "movzx eax, al" << endl;
                        } else if (token.value == ">=") {
                            cout << "cmp eax, ebx" << endl;
                            cout << "setge al" << endl;
                            cout << "movzx eax, al" << endl;
                        } else if (token.value == "==") {
                            cout << "cmp eax, ebx" << endl;
                            cout << "sete al" << endl;
                            cout << "movzx eax, al" << endl;
                        } else if (token.value == "!=") {
                            cout << "cmp eax, ebx" << endl;
                            cout << "setne al" << endl;
                            cout << "movzx eax, al" << endl;
                        }

                        cout << "push eax" << endl;
                    }

                }
                cout << "pop eax" << endl;  // 将最终结果弹出到 eax
                cout << "mov DWORD PTR [ebp-" << offset[var] << "], eax" << endl;
                s.clear();
                cout << "#finished" << endl;
            }
        }
    }
}
