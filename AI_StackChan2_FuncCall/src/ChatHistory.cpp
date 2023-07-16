#include "ChatHistory.h"

ChatHistory::ChatHistory(int _max_history) {
  max_history = _max_history;
}

ChatHistory::~ChatHistory() {
  
}

void ChatHistory::pop_front(){
  chatHistory_role.pop_front();
  chatHistory_funcName.pop_front();
  chatHistory_content.pop_front();
}

void ChatHistory::push_back(String role, String funcName, String content){
  chatHistory_role.push_back(role);
  chatHistory_funcName.push_back(funcName);
  chatHistory_content.push_back(content);

  // チャット履歴が最大数を超えた場合、古い質問と回答を削除
  if (get_size() > max_history)
  {
    pop_front();
  }
}

int ChatHistory::get_size(){
  return chatHistory_role.size();
}

String ChatHistory::get_role(int i){
  return chatHistory_role[i];
}

String ChatHistory::get_funcName(int i){
  return chatHistory_funcName[i];
}

String ChatHistory::get_content(int i){
  return chatHistory_content[i];
}

void ChatHistory::clear(){
  chatHistory_role.clear();
  chatHistory_funcName.clear();
  chatHistory_content.clear();
}