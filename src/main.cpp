#include <Arduino.h>
#include <malloc.h>
#include <string.h>
#include <cstring>
#include <locale.h>
#include <AzureIotHub.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define LEN 99
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_MOSI 16
#define OLED_CLK 13
#define OLED_DC 4
#define OLED_CS 23
#define OLED_RESET 0

#define ADC_1_1 2
#define ADC_2_1 15
#define ADC_3_1 12
#define ADC_4_1 14
#define ADC_5_1 27
double x[2];                    // 缓存解方程的解
int fx[3];                      // 缓存解方程参数
int df[3];                      // 缓存画图方程参数
int position[2];                // 缓存矩形图像位置参数
double a;
String inputString = "";        // 缓存字符串
boolean stringComplete = false; // 是否string已经完成缓存
int modeFlag = 0;               // 0为主页 1为计算器 2为函数绘制 3为方程式计算
float ADC1 = 0.0;               // 换存ADC值

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

double calculate(char *c)
{
  // c是存计算式字符串的指针
  double result = 0; //最终返回变量
  int c_len = strlen(c);
  int i, k; // i当过数组下标
  double temp, j;
  double opnd[LEN]; //操作数栈,能存10个操作数,包括小数点
  int opnd_index = 0;
  char optr[LEN] = ""; //操作符栈

  int optr_index = 0;
  /*当 ) 进入栈的时候，会将 (……)里的计算式都处理了,
  左栈存一个两个数结合的新数,右栈指针指向(,
  紧接着再右栈再输入 +-×÷* 就不许先不需要计算了,直接替换 ) */
  int CalByButton_r = 0;    //等于0是 ) 处理式子之前
  char num_stack[LEN] = ""; //临时数栈
  int num_stack_index = 0;  //临时数下标
  double num_temp = 0;      //临时数
  optr[0] = 0;
  //注意：while的最后一次运行是 数字的，所以数字下面的判断都不会执行
  while (c_len)
  { //+-×÷.肯不会在字符串开头 (可能在开头，需要处理
    c_len--;
    //*c是数字情况，包括小数点
    if ((*c <= '9' && *c >= '0') || (*c == '.'))
    {
      num_stack[num_stack_index] = *c;
      num_stack_index++;
      c++;
      continue;
    }
    //*c是 + 情况
    if (*c == '+')
    {
      num_stack_index--;
      if (num_stack_index == -1)
      { // ) 已经提取一遍左面了，) 后的运算符 就不用再提左面了
        num_stack_index = 0;
        goto c1;
      }
      //将+前一个数抽出存入opnd，num_temp是从字符串中成功抽取的一个数,并且已经变成double
      for (i = num_stack_index, j = 1; i >= 0; i--, j *= 10)
      {
        if (num_stack[i] != '.')
          num_temp += ((num_stack[i] - '0') * j);

        if (num_stack[i] == '.')
        {
          for (k = num_stack_index - i; k > 0; k--)
          {
            num_temp /= 10;
          }
          j = 0.1; //下一次j还是1
        }
      }
      opnd[opnd_index] = num_temp;
      opnd_index++;
      num_temp = 0;        //临时数初始化
      num_stack_index = 0; //临时栈下标初始化
    c1:
      //存 + 的一些操作
      if (optr[optr_index] == 0)
      {                         //如果optr为空,直接进入，之后直到到最终计算之前都不会为空了
        optr[optr_index] = '+'; //先不++，因为下面是判断上一个入栈负号
      }                         //如果optr不为空，+ 是 直接抽出里面的进行运算，再存
      else if (optr[optr_index] == '+')
      {                  //左提俩求和=新数，将新数存进opnd,+号不错废掉
        opnd_index -= 2; //减去多加的 和 二合一之后位置为下面那个
        temp = opnd[opnd_index] + opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++; //

        optr[optr_index] = '+'; //直接替换掉原来下标为这个的符号
      }
      else if (optr[optr_index] == '-')
      {
        opnd_index -= 2;
        temp = opnd[opnd_index] - opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;

        optr[optr_index] = '+'; //直接替换掉原来下标为这个的符号
      }
      else if (optr[optr_index] == '*')
      { //+遇到
        opnd_index -= 2;
        temp = opnd[opnd_index] * opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;
        while (1)
        { //处理 6+8-4*2+6-10 的 bug
          if (optr[optr_index - 1] == '+' || optr[optr_index - 1] == '-')
          {
            optr_index--;
            opnd_index -= 2;
            switch (optr[optr_index])
            {
            case '+':
              opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
              break;
            case '-':
              opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
              break;
            }
            opnd_index++;
          }
          break;
        }
        optr[optr_index] = '+'; //直接替换掉原来下标为这个的符号
      }
      else if (optr[optr_index] == '/')
      {
        opnd_index -= 2;
        temp = opnd[opnd_index] / opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;

        while (1)
        {                         //处理 6+8-4/2+6-10 的 bug
          if (optr_index - 1 < 0) //防止数组越界
            break;
          if (optr[optr_index - 1] == '+' || optr[optr_index - 1] == '-')
          {
            optr_index--;
            opnd_index -= 2;
            switch (optr[optr_index])
            {
            case '+':
              opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
              break;
            case '-':
              opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
              break;
            }
            opnd_index++;
          }
          break;
        }
        optr[optr_index] = '+'; //直接替换掉原来下标为这个的符号
      }
      else if (optr[optr_index] == '(')
      { //如果是 ( 直接存进去
        if (CalByButton_r == 1)
        { //这个情况是遇到 由 ) 计算后 的 ( ; 不是输入 ) 前的 (
          if ((optr_index - 1) >= 0)
          { //处理 6*(1-2*2)+3会=0 的问题 存之前应该再判断一下 原 ( 下的符号*/
            optr_index--;
            opnd_index -= 2;
            switch (optr[optr_index])
            { //这是运算符栈顶符号，此时 ) 没存进去
            case '+':
              opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
              break;
            case '-':
              opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
              break;
            case '*':
              opnd[opnd_index] = opnd[opnd_index] * opnd[opnd_index + 1];
              break;
            case '/':
              opnd[opnd_index] = opnd[opnd_index] / opnd[opnd_index + 1];
              break;
            //下面这个case是处理 ( 下还是 ( 的情况
            case '(':
              optr_index++;
              opnd_index++;
              break; //只需将optr_index和opnd_index归位即可
            }
            opnd_index++;
            optr[optr_index] = '+';
            CalByButton_r = 0; //归位
            while (1)
            {
              if (optr_index - 1 < 0) //防止数组越界
                break;
              if (optr[optr_index - 1] == '+' || optr[optr_index - 1] == '-')
              {
                optr_index--;
                opnd_index -= 2;
                switch (optr[optr_index])
                {
                case '+':
                  opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
                  break;
                case '-':
                  opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
                  break;
                }
                opnd_index++;
              }
              optr[optr_index] = '+';
              break;
            }
            c++;
            continue;
          }
          optr[optr_index] = '+';
          CalByButton_r = 0; //归位
          c++;
          continue;
        }
        optr_index++;
        optr[optr_index] = '+';
      }
      c++;
      continue;
    }
    //*c是 - 情况，和 + 一样  负号处理在 符号(中
    if (*c == '-')
    {
      num_stack_index--;
      if (num_stack_index == -1)
      { // ) 已经提取一遍左面了，) 后的运算符 就不用再提左面了
        num_stack_index = 0;
        goto c2;
      }
      //将-前一个数抽出存入opnd，num_temp是从字符串中成功抽取的一个数,并且已经变成double
      for (i = num_stack_index, j = 1; i >= 0; i--, j *= 10)
      {
        if (num_stack[i] != '.')
          num_temp += ((num_stack[i] - '0') * j);

        if (num_stack[i] == '.')
        {
          for (k = num_stack_index - i; k > 0; k--)
          {
            num_temp /= 10;
          }
          j = 0.1; //下一次j还是1
        }
      }
      opnd[opnd_index] = num_temp;
      opnd_index++;
      num_temp = 0;        //临时数初始化
      num_stack_index = 0; //临时栈下标初始化
    c2:
      //存 - 的一些操作
      if (optr[optr_index] == 0)
      {                         //如果optr为空,直接进入
        optr[optr_index] = '-'; //先不++，因为下面是判断上一个入栈负号
      }                         //如果optr不为空，+ 是 直接抽出里面的进行运算，再存
      else if (optr[optr_index] == '+')
      {                  //左提俩求和=新数，将新数存进opnd,+号不错废掉
        opnd_index -= 2; //减去多加的 和 二合一之后位置为下面那个
        temp = opnd[opnd_index] + opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++; //

        optr[optr_index] = '-'; //直接替换掉原来下标为这个的符号
      }
      else if (optr[optr_index] == '-')
      {
        opnd_index -= 2;
        temp = opnd[opnd_index] - opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;

        optr[optr_index] = '-'; //直接替换掉原来下标为这个的符号
      }
      else if (optr[optr_index] == '*')
      { //+遇到
        opnd_index -= 2;
        temp = opnd[opnd_index] * opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;
        while (1)
        { //处理 6+8-4/2+6-10 的 bug
          if (optr[optr_index - 1] == '+' || optr[optr_index - 1] == '-')
          {
            optr_index--;
            opnd_index -= 2;
            switch (optr[optr_index])
            {
            case '+':
              opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
              break;
            case '-':
              opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
              break;
            }
            opnd_index++;
          }
          break;
        }
        optr[optr_index] = '-'; //直接替换掉原来下标为这个的符号
      }
      else if (optr[optr_index] == '/')
      {
        opnd_index -= 2;
        temp = opnd[opnd_index] / opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;
        while (1)
        {                         //处理 6+8-4/2+6-10 的 bug
          if (optr_index - 1 < 0) //防止数组越界
            break;
          if (optr[optr_index - 1] == '+' || optr[optr_index - 1] == '-')
          {
            optr_index--;
            opnd_index -= 2;
            switch (optr[optr_index])
            {
            case '+':
              opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
              break;
            case '-':
              opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
              break;
            }
            opnd_index++;
          }
          break;
        }
        optr[optr_index] = '-'; //直接替换掉原来下标为这个的符号
      }
      else if (optr[optr_index] == '(')
      { //如果是 ( 直接存进去
        if (CalByButton_r == 1)
        { //这个情况是 由 ) 计算后 的 ( 遗留
          if ((optr_index - 1) >= 0)
          { //处理 6*(1-2*2)+3会=0 的问题 /*存之前应该再判断一下 原 ( 下的符号*/
            optr_index--;
            opnd_index -= 2;
            switch (optr[optr_index])
            { //这是运算符栈顶符号，此时 ) 没存进去
            case '+':
              opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
              break;
            case '-':
              opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
              break;
            case '*':
              opnd[opnd_index] = opnd[opnd_index] * opnd[opnd_index + 1];
              break;
            case '/':
              opnd[opnd_index] = opnd[opnd_index] / opnd[opnd_index + 1];
              break;
            case '(':
              optr_index++;
              opnd_index++;
              break;
              //如果是 ( 下还是 ( 只需将optr_index和opnd_index归位即可
            }
            opnd_index++;
            optr[optr_index] = '-';
            CalByButton_r = 0; //归位
            c++;
            continue;
          }
          optr[optr_index] = '-';
          CalByButton_r = 0; //归位
          while (1)
          {
            if (optr_index - 1 < 0) //防止数组越界
              break;
            if (optr[optr_index - 1] == '+' || optr[optr_index - 1] == '-')
            {
              optr_index--;
              opnd_index -= 2;
              switch (optr[optr_index])
              {
              case '+':
                opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
                break;
              case '-':
                opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
                break;
              }
              opnd_index++;
            }
            optr[optr_index] = '-';
            break;
          }
          c++;
          continue;
        }
        optr_index++;
        optr[optr_index] = '-';
      }
      c++;
      continue;
    }
    //*c是 × 情况
    if (*c == '*')
    {
      num_stack_index--;
      if (num_stack_index == -1)
      { // ) 已经提取一遍左面了，) 后的运算符 就不用再提左面了
        num_stack_index = 0;
        goto c3;
      }
      //将*前一个数抽出存入opnd，num_temp是从字符串中成功抽取的一个数,并且已经变成double
      for (i = num_stack_index, j = 1; i >= 0; i--, j *= 10)
      {
        if (num_stack[i] != '.')
          num_temp += ((num_stack[i] - '0') * j);

        if (num_stack[i] == '.')
        {
          for (k = num_stack_index - i; k > 0; k--)
          {
            num_temp /= 10;
          }
          j = 0.1; //下一次j还是1
        }
      }
      opnd[opnd_index] = num_temp;
      opnd_index++;
      num_temp = 0;        //临时数初始化
      num_stack_index = 0; //临时栈下标初始化
    //存 * 的一些操作
    c3:
      if (optr[optr_index] == 0)
      {                         //如果optr为空,直接进入
        optr[optr_index] = '*'; //先不++，因为下面是判断上一个入栈负号
      }                         //如果optr不为空
      else if (optr[optr_index] == '+')
      { //遇到+号不运算直接存进去
        optr_index++;
        optr[optr_index] = '*';
      }
      else if (optr[optr_index] == '-')
      {
        optr_index++;
        optr[optr_index] = '*';
      }
      else if (optr[optr_index] == '*')
      { //*遇到*,左提俩=新数，新数放入,*放入原来*位置,也就是不用操作
        opnd_index -= 2;
        temp = opnd[opnd_index] * opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;
      }
      else if (optr[optr_index] == '/')
      {
        opnd_index -= 2;
        temp = opnd[opnd_index] / opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;

        optr[optr_index] = '*'; //*替换/的位置
      }
      else if (optr[optr_index] == '(')
      { //如果是 ( 直接存进去
        if (CalByButton_r == 1)
        { //这个情况是 由 ) 计算后 的 (
          if ((optr_index - 1) >= 0)
          { //处理 6*(1-2*2)+3会=0 的问题 /*存之前应该再判断一下 原 ( 下的符号*/
            if (optr[optr_index - 1] != '+' && optr[optr_index - 1] != '-')
            { //处理6-(5+2)*6 *入之前 把 - 号给算了
              optr_index--;
              opnd_index -= 2;
              switch (optr[optr_index])
              { //这是运算符栈顶符号，此时 ) 没存进去
              case '+':
                opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
                break;
              case '-':
                opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
                break;
              case '*':
                opnd[opnd_index] = opnd[opnd_index] * opnd[opnd_index + 1];
                break;
              case '/':
                opnd[opnd_index] = opnd[opnd_index] / opnd[opnd_index + 1];
                break;
              case '(':
                optr_index++;
                opnd_index++;
                break;
                //如果是 ( 下还是 ( 只需将optr_index和opnd_index归位即可
              }
              opnd_index++;
              optr[optr_index] = '*';
              CalByButton_r = 0; //归位
              c++;
              continue;
            }
          }
          optr[optr_index] = '*';
          CalByButton_r = 0; //归位
          c++;
          continue;
        }
        optr_index++;
        optr[optr_index] = '*';
      }
      c++;
      continue;
    }
    //*c是 / 情况
    if (*c == '/')
    {
      num_stack_index--;
      if (num_stack_index == -1)
      { // ) 已经提取一遍左面了，) 后的运算符 就不用再提左面了
        num_stack_index = 0;
        goto c4;
      }
      //将*前一个数抽出存入opnd，num_temp是从字符串中成功抽取的一个数,并且已经变成double
      for (i = num_stack_index, j = 1; i >= 0; i--, j *= 10)
      {
        if (num_stack[i] != '.')
          num_temp += ((num_stack[i] - '0') * j);

        if (num_stack[i] == '.')
        {
          for (k = num_stack_index - i; k > 0; k--)
          {
            num_temp /= 10;
          }
          j = 0.1; //下一次j还是1
        }
      }
      opnd[opnd_index] = num_temp;
      opnd_index++;
      num_temp = 0;        //临时数初始化
      num_stack_index = 0; //临时栈下标初始化
    c4:
      //存 / 的一些操作
      if (optr[optr_index] == 0)
      {                         //如果optr为空,直接进入
        optr[optr_index] = '/'; //先不++，因为下面是判断上一个入栈负号
      }                         //如果optr不为空
      else if (optr[optr_index] == '+')
      { //遇到+号不运算直接存进去
        optr_index++;
        optr[optr_index] = '/';
      }
      else if (optr[optr_index] == '-')
      {
        optr_index++;
        optr[optr_index] = '/';
      }
      else if (optr[optr_index] == '*')
      {
        opnd_index -= 2;
        temp = opnd[opnd_index] * opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;

        optr[optr_index] = '/'; // /替换*的位置
      }
      else if (optr[optr_index] == '/')
      { // /遇到/,左提俩=新数，新数放入,/放入原来/位置,也就是不用操作
        opnd_index -= 2;
        temp = opnd[opnd_index] / opnd[opnd_index + 1];
        opnd[opnd_index] = temp;
        opnd_index++;
      }
      else if (optr[optr_index] == '(')
      { //如果是 ( 直接存进去
        if (CalByButton_r == 1)
        { //这个情况是 由 ) 计算后 的 (
          if ((optr_index - 1) >= 0)
          { //处理 6*(1-2*2)+3会=0 的问题 /*存之前应该再判断一下 原 ( 下的符号*/
            if (optr[optr_index - 1] != '+' && optr[optr_index - 1] != '-')
            { //处理6-(5+2)/6 /入之前 把 - 号给算了
              optr_index--;
              opnd_index -= 2;
              switch (optr[optr_index])
              { //这是运算符栈顶符号，此时 ) 没存进去
              case '+':
                opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
                break;
              case '-':
                opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
                break;
              case '*':
                opnd[opnd_index] = opnd[opnd_index] * opnd[opnd_index + 1];
                break;
              case '/':
                opnd[opnd_index] = opnd[opnd_index] / opnd[opnd_index + 1];
                break;
              case '(':
                optr_index++;
                opnd_index++;
                break;
                //如果是 ( 下还是 ( 只需将optr_index和opnd_index归位即可
              }
              opnd_index++;
              optr[optr_index] = '/';
              CalByButton_r = 0; //归位
              c++;
              continue;
            }
          }
          optr[optr_index] = '/';
          CalByButton_r = 0; //归位
          c++;
          continue;
        }
        optr_index++;
        optr[optr_index] = '/';
      }
      c++;
      continue;
    }
    //是 ( 情况
    if (*c == '(')
    { //直接入操作符栈
      if (optr_index == 0)
      {
        if (optr[0] != '+' && optr[0] != '-' && optr[0] != '*' && optr[0] != '/' && optr[0] != '(')
        {
          optr[optr_index] = '(';
          c++;
          continue;
        }
      }
      optr_index++;
      optr[optr_index] = '(';
      c++;
      continue;
    }
    //是 ) 情况
    if (*c == ')')
    {
      num_stack_index--;
      //将*前一个数抽出存入opnd，num_temp是从字符串中成功抽取的一个数,并且已经变成double
      for (i = num_stack_index, j = 1; i >= 0; i--, j *= 10)
      {
        if (num_stack[i] != '.')
          num_temp += ((num_stack[i] - '0') * j);

        if (num_stack[i] == '.')
        {
          for (k = num_stack_index - i; k > 0; k--)
          {
            num_temp /= 10;
          }
          j = 0.1; //下一次j还是1
        }
      }
      opnd[opnd_index] = num_temp;
      opnd_index++;
      num_temp = 0;        //临时数初始化
      num_stack_index = 0; //临时栈下标初始化

      while (1)
      { //用while是处理 这种情况 (1-5*2) 也就是括号内的多项式
        opnd_index -= 2;
        switch (optr[optr_index])
        { //这是运算符栈顶符号，此时 ) 没存进去
        case '+':
          opnd[opnd_index] = opnd[opnd_index] + opnd[opnd_index + 1];
          break;
        case '-':
          opnd[opnd_index] = opnd[opnd_index] - opnd[opnd_index + 1];
          break;
        case '*':
          opnd[opnd_index] = opnd[opnd_index] * opnd[opnd_index + 1];
          break;
        case '/':
          opnd[opnd_index] = opnd[opnd_index] / opnd[opnd_index + 1];
          break;
        }
        opnd_index++;
        optr_index--; //运算符栈下降一位
        if (optr[optr_index] == '(')
        {
          CalByButton_r = 1; //上面有说明
          break;
        }
      }
    }

    c++; //指针在最后时刻移动
  }

  ///////////////////////////////////////////
  num_stack_index--;
  //将+前一个数抽出存入opnd，num_temp是从字符串中成功抽取的一个数,并且已经变成double
  for (i = num_stack_index, j = 1; i >= 0; i--, j *= 10)
  {
    if (num_stack[i] != '.')
      num_temp += ((num_stack[i] - '0') * j);

    if (num_stack[i] == '.')
    {
      for (k = num_stack_index - i; k > 0; k--)
      {
        num_temp /= 10;
      }
      j = 0.1; //下一次j还是1
    }
  }
  opnd[opnd_index] = num_temp;
  opnd_index++;
  num_temp = 0;        //临时数初始化
  num_stack_index = 0; //临时栈下标初始化

  i = optr_index;
  do
  {
    switch (optr[i])
    {
    case '+':
      opnd[i] = opnd[i] + opnd[i + 1];
      break;
    case '-':
      opnd[i] = opnd[i] - opnd[i + 1];
      break;
    case '*':
      opnd[i] = opnd[i] * opnd[i + 1];
      break;
    case '/':
      opnd[i] = opnd[i] / opnd[i + 1];
      break;
    }
    i--;
  } while (i != -1);
  result = opnd[0];

  return result;
}

void initmenu(void)
{
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("1. Calculator");
  display.setCursor(0, 8);
  display.println("2. Draw Input");
  display.setCursor(0, 16);
  display.println("3. Equation");
  display.setCursor(0, 24);
  display.println("4. Key Down");
  display.display(); // Show initial text
  delay(100);
}

void scrolltext(String text)
{
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(text);

  display.display(); // Show initial text
  delay(100);
}

void Calculator(String line)
{
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Calculator");
  display.setCursor(0, 8);
  display.println(line);
  display.display();
  delay(100);
}

void DrawInput(String line)
{
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Draw function");
  display.display();
  display.println(line);
  display.display();
  delay(100);
}

double DrawFunction(int a, int b, int c, double x){
    return (-c-a*x)/b;
}

void DrawRect(int positionX, int positionY){
  display.clearDisplay();
  display.fillRect(12 * (positionX), 12 * (positionY), 12, 12, SSD1306_WHITE);
  display.display();
  delay(100);
}

void EquationFunction(String line)
{
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Equation");
  display.setCursor(0, 8);
  char p[15];
  int i;
  for(i = 0; i < line.length(); i++){
    p[i] = line[i];
  }
  p[i] = '\0';
  for (int j = 0; j < line.length(); j++)
  {
    if (p[j] == 'X')
    {
      display.setTextSize(2);
      display.print('x');
      display.setTextSize(1);
      display.print('2');
    }else{
      display.setTextSize(2);
      display.print(p[j]);
    }
  }
  display.display();
  delay(100);
}

void EquationAnswer(double x0, double x1)
{
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("EquationAnswer");
  if (!(x0 == 0 && x1 == 0))
  {
    display.setCursor(0, 8);
    display.print("x0:");
    display.print(x0);
    display.setCursor(0, 16);
    display.print("x1:");
    display.print(x1);
  }
  display.display();
  delay(100);
}

void EquationAlgorithm(float x2, float x1, float x0)
{
  double n, m;
  if (x2 != 0)
  {
    n = (x1 * x1) - (4.0 * x2 * x0);
    if (n < 0)
    {
      x[0] = 999.99;
      x[1] = 999.99;
    }
    else
    {
      m = sqrt(n);
      x[0] = (-x1 + m) / (2.0 * x2);
      x[1] = (-x1 - m) / (2.0 * x2);
    }
  }
}

float GetAdc(int adc){
  adc = analogRead(ADC_1_1);
  float volt = adc / 1024.0 * 5 / 4;
  return volt;
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(ADC_1_1, INPUT);


  if (!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("oled disconnected"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  initmenu();

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);
  display.clearDisplay();
}

void loop()
{

  // ADC1 = GetAdc(ADC_1_1);
  // Serial.print("ADC1: ");
  // Serial.println(ADC1);
  //delay(500);
  // put your main code here, to run repeatedly:
  if (Serial.available())
  {
    while (Serial.available())
    {
      /* code */
      char cmd = Serial.read();
      int ascii = int(cmd);
      if (cmd == ';')
      {
        /* code */
        modeFlag = 0;
        inputString = "";
        x[0] = 0;
        x[1] = 0;
        fx[0] = 0;
        fx[1] = 0;
        fx[2] = 0;
        df[0] = 0;
        df[1] = 0;
        df[2] = 0;
        initmenu();
      }
      switch (modeFlag)
      {
      case 0: //主页 0
        switch (ascii)
        {
        case 49: //进入计算器
          /* code */
          modeFlag = 1;
          Calculator("");
          break;
        case 50: //进入函数图像绘制
          modeFlag = 2;
          DrawInput("");
          break;
        case 51: //进入方程式计算
          modeFlag = 3;
          EquationFunction("");
          break;
        case 52:
          modeFlag = 4;
          DrawRect(0,0);      // Draw rectangles (outlines)
        default:
          break;
        }
        break;
      case 1:
        inputString += cmd;
        if (cmd == '=')
        {
          /* code */
          char *charResult = "";
          charResult = (char *)inputString.c_str();
          double result = calculate(charResult);
          Calculator(String(result));
          inputString = "";
        }
        else
        {
          Calculator(inputString);
        }
        break;
      case 2:
        if (cmd == '=')
        {
          
          char q[15];
          int i;
          for(i = 0; i < inputString.length(); i++){
            q[i] = inputString[i];
          }
          q[i] = '\0';

          
          for (int j = 0; j < inputString.length(); j++)
          {
            if (q[j] == 'x')
            {
              if (j == 0)
              {
                df[0] = 1;
              }
              else if (j == 1)
              {
                if (q[j - 1] == '-')
                {
                  df[0] = -1;
                }
                else
                {
                  df[0] = (q[j-1]-48);
                }
              }
              else if(j == 2){
                df[0] = -((int)(q[1])-48);
              }
            }
            if (q[j] == 'y')
            {
              if (q[j-1] == '+')
              {
                df[1] = 1;
              }else if (q[j-1] == '-')
              {
                df[1] = -1;
              }
              else if (q[j-2] == '+')
              {
                df[1] = q[j-1]-48;
              }else if(q[j-2] == '-'){
                df[1] = -((int)(q[j-1]) - 48);
              }
            }
            if(q[inputString.length() - 1] == 'x'){
              df[2] = 0;
            }
            else
            {
              if (q[inputString.length()-2] == '+')
              {
                df[2] = q[inputString.length()-1]-48;
              }
              else if (q[inputString.length() - 2] == '-')
              {
                df[2] = -(q[inputString.length()-1]-48);
              }
            }
          }
          display.clearDisplay();
          display.drawLine(64,0, 64, 64, SSD1306_WHITE);
          display.drawLine(0,32, 128, 32, SSD1306_WHITE);
          for(double i = -64; i < 128; i++){
            double y = DrawFunction(df[0], df[1], df[2], i/10);
            double xm = i/10;
            display.drawPixel(64 + i, 32 - y*10, SSD1306_WHITE);
          }
          display.display();
        }
        else if((ascii >= 48 && ascii <= 57) || ascii == 43 || ascii == 45)
        {
          inputString += cmd;
          DrawInput(inputString);
        }
        else if (ascii == 120)
        {
          inputString += "x";
          DrawInput(inputString);
        }
        else if (ascii == 121)
        {
          inputString += "y";
          DrawInput(inputString);
        }
        break;
      case 3:
        if (cmd == '=')
        {
          /* code */
          char q[15];
          int i;
          for(i = 0; i < inputString.length(); i++){
            q[i] = inputString[i];
          }
          q[i] = '\0';
          for (int j = 0; j < inputString.length(); j++)
          {
            if (q[j] == 'X')
            {
              if (j == 0)
              {
                fx[0] = 1;
              }
              else if (j == 1)
              {
                if (q[j - 1] == '-')
                {
                  fx[0] = -1;
                }
                else
                {
                  fx[0] = (q[j-1]-48);
                }
              }
              else if(j == 2){
                fx[0] = -((int)(q[1])-48);
              }
            }
            if (q[j] == 'x')
            {
              if (q[j-1] == '+')
              {
                fx[1] = 1;
              }else if (q[j-1] == '-')
              {
                fx[1] = -1;
              }
              else if (q[j-2] == '+')
              {
                fx[1] = q[j-1]-48;
              }else if(q[j-2] == '-'){
                fx[1] = -((int)(q[j-1]) - 48);
              }
            }
            if(q[inputString.length() - 1] == 'x'){
              fx[2] = 0;
            }
            else
            {
              if (q[inputString.length()-2] == '+')
              {
                fx[2] = q[inputString.length()-1]-48;
              }
              else if (q[inputString.length() - 2] == '-')
              {
                fx[2] = -(q[inputString.length()-1]-48);
              }
            }
          }
          EquationAlgorithm(fx[0], fx[1], fx[2]);
          EquationAnswer(x[0], x[1]);
          inputString = "";
        }
        else if((ascii >= 48 && ascii <= 57) || ascii == 43 || ascii == 45)
        {
          inputString += cmd;
          EquationFunction(inputString);
        }
        else if (ascii == 120)
        {
          inputString += "x";
          EquationFunction(inputString);
        }
        else if (ascii == 88)
        {
          inputString += "X";
          EquationFunction(inputString);
        }
        break;
      case 4:
        int a = cmd - 48;
        Serial.print("a : ");
        Serial.println(a);
        DrawRect(a%5, a/5);
        break;
      }
    }
  }
}