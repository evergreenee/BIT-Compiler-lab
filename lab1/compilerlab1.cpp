#include<bits/stdc++.h>
using namespace std;

int cnt; //变量数
unordered_map<string,int>offset; //变量对应偏置值

// 单词
struct Token {
    string type; // "keyword", "identifier", "constant", "operator"
    string value;
};

void generateCode(const vector<Token>& tokens); //生成代码
vector<Token> lexicalAnalysis(const string& line); //词法分析
void operations(string s); //处理操作符

void operations(string s){
    cout << "pop ebx" << endl;
    cout << "pop eax" << endl;
    if(s[0] == '+') {
        cout << "add eax, ebx" << endl;
    }
    else if(s[0] == '-'){
        cout << "sub eax, ebx" << endl;
    }
    else if(s[0] == '*'){
        cout << "mov ecx, 0" << endl;
        cout << "imul eax,ebx" << endl;
    }
    else if(s[0] == '/'){
        cout << "cdq" << endl;
        cout << "idiv ebx" << endl;
    }
    cout << "push eax" << endl;
}

vector<Token> lexicalAnalysis(const string& line) {
    vector<Token> tokens;
    tokens.clear();
    regex keywordRegex("\\b(int|return)\\b");
    regex identifierRegex("([a-zA-Z])");
    regex constantRegex("(\\d+)");
    regex operatorRegex("[=\\+\\*/\\(\\)-]");

    istringstream iss(line);
    string token;
    while (iss >> token) {
        //cout << token << endl;
        if (regex_match(token, keywordRegex)) {
            tokens.push_back({"keyword", token});
        } else if (regex_match(token, identifierRegex)) {
            tokens.push_back({"identifier", token});
        } else if (regex_match(token, constantRegex)) {
            tokens.push_back({"constant", token});
        } else if (regex_match(token, operatorRegex)) {
            tokens.push_back({"operator", token});
        }
    }
    return tokens;
}

void generateCode(const vector<Token>& tokens){
//    for (const auto& token : tokens) {
//        cout << "Type: " << token.type << ", Value: \"" << token.value << "\"" << endl;
//    }
    //第一个token为关键字int
    if(tokens[0].type == "keyword" && tokens[0].value == "int"){
        offset[tokens[1].value] = (++cnt) * 4;
        cout << "mov DWORD PTR [ebp-" << offset[tokens[1].value] << "], 0     # int " << tokens[1].value << endl;
    }
    //第一个token为关键字return
    else if(tokens[0].type == "keyword" && tokens[0].value == "return"){
        cout << "mov eax, DWORD PTR [ebp-" << offset[tokens[1].value] << "]      # return " << tokens[1].value << endl;
    }
    //一行只有3个token且第二个为操作符=，则为赋值语句
    else if(tokens.size() == 3 && tokens[1].type == "operator" && tokens[1].value == "="){
        //第三个token类型为常数时，直接赋值
        if(tokens[2].type == "constant"){
            cout << "mov DWORD PTR [ebp-" << offset[tokens[0].value] << "], " << tokens[2].value << "     # " << tokens[0].value << " = " << tokens[2].value << endl;
        }
        //否则为变量赋值
        else{
            cout << "push eax" << endl;
            cout << "mov eax, DWORD PTR [ebp-" << offset[tokens[2].value] << "]      # " << tokens[0].value << " = " << tokens[2].value << endl;
            cout << "mov DWORD PTR [ebp-" << offset[tokens[0].value] << "], eax    " << endl;
            cout << "pop eax" << endl;
        }
    }
    //否则为运算赋值
    else{
        //初始化空栈
        stack<string>s;
        while(!s.empty()){
            s.pop();
        }
        //压入一个(便于确定计算流程
        s.push("(");
        bool done = false; //计算完成标志
        int _cnt = 1; //判别已处理单词数量(从第三个开始，即下标为2)

        while(!done){
            _cnt++; //为了使_cnt == tokens.size()判断有效，在循环开始时就自增，因此_cnt初始化为1

            if(_cnt == tokens.size()){
                s.push(")");
                done = true;
            }
            else if(tokens[_cnt].type == "constant"){
                cout << "mov eax," << tokens[_cnt].value.c_str() << endl;
                cout << "push eax" << endl;
                continue;
            }
            else if(tokens[_cnt].type == "identifier"){
                cout << "push DWORD PTR [ebp-" << offset[tokens[_cnt].value] << "]" << endl;
                continue;
            }
            else if(tokens[_cnt].type == "operator"){
                s.push(tokens[_cnt].value);
            }
            string op1,op2;
            while(!s.empty()){
                op2 = s.top();
                s.pop();
                op1 = s.top();
                if(op2 == "("){
                    s.push(op2);
                    break;
                }
                if(op1 == "(" && op2 == ")"){
                    s.pop();
                    break;
                }
                else if (op1 == "(" && (op2 == "+" || op2 == "-")){
                    s.push(op2);
                    break;
                }
                else if ((op1 == "(" || op1 =="+" || op1 =="-") && (op2 == "*" || op2 == "/")){
                    s.push(op2);
                    break;
                }
                operations(op1);
                s.pop();
                s.push(op2);
            }
        }
        cout << "pop eax" << endl;
        cout << "mov DWORD PTR [ebp-" << offset[tokens[0].value] << "], eax" << endl;
    }
}

int main(int argc, char* argv[]) {
    ifstream sourceFile(argv[1]);
    //ifstream sourceFile("C:\\Users\\Lenovo\\Desktop\\compiler\\1.txt");

    if (!sourceFile.is_open()) {
        cerr << "无法打开源文件。" << endl;
        return 1;
    }
    //读取文件内容存入字符串inputCode中
    stringstream buffer;
    buffer << sourceFile.rdbuf();
    string inputCode = buffer.str();
    //去除换行和回车
    inputCode.erase(remove(inputCode.begin(), inputCode.end(), '\n'), inputCode.end());
    inputCode.erase(remove(inputCode.begin(), inputCode.end(), '\r'), inputCode.end());
    //关闭文件
    sourceFile.close();

    //cout << inputCode << endl;

    istringstream iss(inputCode);
    string line;
    vector<Token> ts;
    //按分号分割内容，并逐行进行处理
    while (getline(iss, line, ';')) {
        //cout << line << endl;
        ts = lexicalAnalysis(line); //词法分析得到tokens
        generateCode(ts);
    }

    return 0;
}
