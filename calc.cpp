#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <ctype.h>
#include <pthread.h>

//empty struct
struct Calc {
};

//calculator implementation
class CalcImpl : public Calc {
  std::map<std::string, int> variables;
  pthread_mutex_t lock;
  int client_fd;
  
  std::vector<std::string> tokenize(const std::string &expr) {
    std::vector<std::string> vec;
    std::stringstream s(expr);
    std::string tok;
    while (s >> tok) {
      vec.push_back(tok);
    }  
    return vec;
  }

  //evaluates "operand"
  bool eval_1(const std::string token, int &result) {
    if (is_operand(token)) {
      result = get_val(token);
      return true;
    }
    return false;
  }

  //evaluates "var = operand"
  bool eval_2(const std::vector<std::string> tokens, int &result) {
    if (is_variable(tokens.at(0))) {
      if (is_operand(tokens.at(2))) {
	std::map<std::string, int>::iterator it = variables.find(tokens.at(0));
	if (it != variables.end()) {
	  it->second = get_val(tokens.at(2));
	  result = get_val(tokens.at(2));
	  return true;
	} else {
	  variables.insert(std::pair<std::string, int>(tokens.at(0), get_val(tokens.at(2))));
	  result = get_val(tokens.at(2));
	  return true;
	}
      }
    } else {
      return false;
    }
    return false;
  }

  //evaluates "operand op operand"
  bool eval_3(const std::vector<std::string> tokens, int &result) {
    int op1, op2;
    if (is_operand(tokens.at(0)) && is_operand(tokens.at(2))) {
      op1 = get_val(tokens.at(0));
      op2 = get_val(tokens.at(2));
    } else {
      return false;
    }
    int op = get_operation(tokens.at(1));
    switch(op) {
    case 1: result = op1 + op2; break;
    case 2: result = op1 - op2; break;
    case 3: result = op1 * op2; break;
    case 4:
      if (op2 == 0) { return false; }
      result = op1 / op2; break;
    default: return false; break;
    }
    return true;
  }

  //evaluates "var = operand op operand"
  bool eval_4(const std::vector<std::string> tokens, int &result) {
    int temp_result;
    std::vector<std::string> temp{tokens.at(2), tokens.at(3), tokens.at(4)};
    if (eval_3(temp, temp_result)) {
      std::map<std::string, int>::iterator it = variables.find(tokens.at(0));
      if (it != variables.end()) {
	it->second = temp_result;
	result = temp_result;
	return true;
      } else {
	variables.insert(std::pair<std::string, int>(tokens.at(0), temp_result));
	result = temp_result;
	return true;
      }
    }
    return false;
  }

  //returns token's int value
  int get_val(const std::string token) {
    if (is_variable(token)) {
      return variables.at(token);
    }
    return std::stoi(token);
  }

  //check if token is operand
  bool is_operand(const std::string token) {
    if (is_variable(token)) {
      pthread_mutex_lock(&lock);
      if (variables.count(token) == 0) {
	return false;
      }
      pthread_mutex_unlock(&lock);
      return true;
    }
    bool first(true);
    for (char c : token) {
      if (first) {
	if (c == '-') {
	  continue;
	}
	first = false;
      }
      if (!isdigit(c)) {
	return false;
      }
    }
    return true;
  }

  //return operation type of token, 0 if not operation
  int get_operation(const std:: string token) {
    if (token.length() > 1) {
      return 0;
    }
    char var = token.at(0);
    return var == '+' ? 1 : var == '-' ? 2 : var == '*' ? 3 : var == '/' ? 4 : 0;
  }

  //checks if token is a variable
  bool is_variable(const std::string token) {
    for (char c : token) {
      if (toupper(c) < 65 || toupper(c) > 90) {
	return false;
      }
    }
    return true;
  }
  
  
public:
  void startMutex() {
    pthread_mutex_init(&lock, NULL);
  }

  void deleteMutex() {
    pthread_mutex_destroy(&lock);
  }

  void set_client_fd(int cfd) {
    client_fd = cfd;
  }

  int get_client_fd() {
    return client_fd;
  }
  
  //evaluates an expression and places result in result
  int evalExpr(const char *expr, int &result) {
    std::vector<std::string> ex(tokenize(expr));
    size_t expr_size = ex.size();
    switch(expr_size) {
    case 1:
      if (eval_1(ex.at(0), result)) { return 1; }
      break;
    case 3:
      if (ex.at(1) == "=") {
	if (eval_2(ex, result)) { return 1; }
      } else if (get_operation(ex.at(1)) != 0) {
	if (eval_3(ex, result)) { return 1; }
      }
      break;
    case 5:
      if (ex.at(1) == "=" && get_operation(ex.at(3)) != 0) {
	if (eval_4(ex, result)) { return 1; }
      }
      break;
    }
    return 0;
  }
};

//creates CalcImpl instance
extern "C" struct Calc *calc_create(void) {
  CalcImpl *calcImpl = new CalcImpl();
  calcImpl->startMutex();
  //return new CalcImpl();
  return calcImpl;
}

//destroys CalcImpl instance
extern "C" void calc_destroy(struct Calc *calc) {
    CalcImpl *obj = static_cast<CalcImpl *>(calc);
    obj->deleteMutex();
    delete obj;
}

//begins expression evaluation
extern "C" int calc_eval(struct Calc *calc, const char *expr, int *result) {
    CalcImpl *obj = static_cast<CalcImpl *>(calc);
    return obj->evalExpr(expr, *result);
}

extern "C" void set_clientfd(struct Calc *calc, int clientfd) {
  CalcImpl *obj = static_cast<CalcImpl *>(calc);
  obj->set_client_fd(clientfd);
}

extern "C" int get_clientfd(struct Calc *calc) {
  CalcImpl *obj = static_cast<CalcImpl *>(calc);
  return obj->get_client_fd();
}
