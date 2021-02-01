#include <mbed.h>
#include "structures.h"
#include "char_search.h"

#define MAXIMUM_BUFFER_SIZE
#define VERBOSE true

eaq_numbers all_nums;

static UnbufferedSerial serial_port(PA_0, PA_1);

EventQueue *queue = mbed_event_queue();

int i = 0;
char buf[30];
bool newLine = false;
bool ready = true;
int x_amount = 0;

int finished = 0;

int curr_queue;

int free_number_rez(char *part);
int x_modif_number(char *part, bool left = true);

int find_simple_x();
int find_difficult_x();

void send_result(int res);
void add_num_simb(int num, char simb, eaq_numbers &list);

void on_rx_interrupt();
void use_data();

void use_data()
{
  memset(&all_nums, 0, sizeof(all_nums));
  //wait_us(100);
  printf("First Line: %.*s\n", i, buf);

  char input[30];                      // i-2
  memcpy(input, buf, strlen(buf) - 2); // Copy first line without "=0"
  input[strlen(buf) - 3] = '\0';       // Give string terminating symbol

  if (VERBOSE)
    printf("Copy: %s\n", input);

  char *firstPart, *secondPart;

  firstPart = strtok(input, "x"); // Split line by symbol 'x'

  if (firstPart)
  {
    if (buf[0] != 'x') // There is a firstPart
    {
      if (VERBOSE)
        printf("FirstPart: %s\n", firstPart);

      x_modif_number(firstPart); // Collect 'x' left modifiers
    }
    else // There is no firstPart
    {
      if (VERBOSE)
        printf("SecondPart: %s\n", firstPart);

      all_nums.x = free_number_rez(firstPart); // Get free nuber on the right
      x_modif_number(firstPart, false);        // Collect 'x' right modifiers
    }
  }

  for (int i = 1; i <= x_amount; i++) // Splitting line while we have x in it
  {
    secondPart = strtok(NULL, "x");

    if (secondPart) // If splitting was successful
    {
      if (VERBOSE)
        printf("SecondPart: %s\n", secondPart);

      if (i == x_amount) // Last part of text
        all_nums.x = free_number_rez(secondPart);

      if (i != x_amount && secondPart[strlen(secondPart) - 1] == '(')
      {
        x_modif_number(secondPart);
      }
      else
      {
        x_modif_number(secondPart, false);
      }
    }
  }

  if (x_amount == 1) // Theres only one x in our equastion
  {
    find_simple_x();
  }
  else // We have more than one x in equation
  {
    find_difficult_x();
  }

  send_result(all_nums.x);

  if (finished < 6)
  {
    finished++;
    i = 0;
    x_amount = 0;
    ready = true;
    memset(&buf, 0, sizeof(buf));
    //printf("Reattaching interrupt! Finished: %d\n", finished);
    serial_port.attach(&on_rx_interrupt, SerialBase::RxIrq); // reattach interrupt
    //return;
  }
  //else
  //  printf("Finished Solving Equations!!!");

  //pc.attach(&onDataReceived, Serial::RxIrq);	// reattach interrupt
}

void on_rx_interrupt()
{
  char c;

  // Read the data
  if (serial_port.read(&c, 1))
  {
    if (!newLine && c == '\n')
    {
      newLine = true;
    }
    else if (newLine)
    {
      if (ready && c != '\n') // Add to buffer
      {
        buf[i] = c;
        i++;

        if (c == 'x')
          x_amount++; // Calculate the amount of x in line
      }
      else // if eol stop
      {
        i++;
        buf[i] = c;
        ready = false;
        newLine = false;

        serial_port.attach(NULL, SerialBase::RxIrq); // detach interrupt
        queue->call(&use_data);                      // process in a non ISR context
      }
    }
  }
}

int main()
{
  // Set desired properties (9600-8-N-1).
  serial_port.baud(9600);
  serial_port.format(
      /* bits */ 8,
      /* parity */ SerialBase::None,
      /* stop bit */ 1);

  // Register a callback to process a Rx (receive) interrupt.
  serial_port.attach(&on_rx_interrupt, SerialBase::RxIrq);
}

void add_num_simb(int num, char simb, eaq_numbers &list)
{
  list.num[list.count] = num;
  list.simb[list.count] = simb;
  list.count++;
}

int free_number_rez(char *part)
{
  char simb;
  int num;

  for (int i = 0; i < strlen(part); i++)
  {
    if (is_char_free_symbol(part[i])) // Check if symbol is - or +
    {
      simb = part[i];
      char tmpNum[10];
      strncpy(tmpNum, part + (i + 1), strlen(part) - 1); // Copy string to the end
      num = atoi(tmpNum);                                // Turn it into our new number

      if (VERBOSE)
        printf("SecondPart Free Number is: %d with symbol: %c\n", num, simb);

      if (simb == '+') // Simbol correction
        return num * (-1);
      else
        return num;
    }
  }
  return 0;
}

int x_modif_number(char *part, bool left)
{
  char simb;
  int num;

  int str_len = strlen(part) - 1;
  int start = 0;

  if (left) // Lines position relative to 'x'
  {
    if (is_char_symbol(part[str_len]))
    {
      if (part[str_len] == '(') // Check if 'x' is covered with ()
        start++;

      simb = part[str_len - start];

      char tmpNum[10];
      strncpy(tmpNum, part, str_len); // Copy number belonging to symbol
      num = atoi(tmpNum);             // Convert it to int

      if (VERBOSE)
        printf("LeftPart Number is: %d with symbol: %c\n", num, simb);

      add_num_simb(num, simb, all_nums); // Add values to our list
    }
  }
  else // We are on the right part of 'x'
  {
    printf("We are on the right part of 'x' First char: %c\n", part[1]);

    if (is_char_symbol(part[0])) // Begining with symbol
    {
      simb = part[0];
      printf("RightPart symbol is: %c\n", simb);

      for (int i = 1; i <= str_len; i++)
      {
        if (is_char_symbol(part[i])) // Find next symbol
        {
          char tmpNum[10];
          strncpy(tmpNum, part + 1, i); // Number is between these symbols
          num = atoi(tmpNum);

          if (VERBOSE)
            printf("RightPart Number is: %d with symbol: %c\n", num, simb);

          add_num_simb(num, simb, all_nums); // Add values to our list

          return 0;
        }
      }
    }
  }
  return 0;
}

int find_simple_x()
{
  for (int i = 0; i < all_nums.count; i++) // Go through our num and simb collection
  {
    if (VERBOSE)
      printf("Loop num = %d, simbl = %c\n", all_nums.num[i], all_nums.simb[i]);

    switch (all_nums.simb[i]) // Modify result based on symbol
    {
    case '*': //or 0
      all_nums.x /= all_nums.num[i];
      break;
    case '/':
      all_nums.x *= all_nums.num[i];
      break;
    case '^':
      all_nums.x = sqrt(all_nums.x);
      break;
    default:
      break;
    }
  }
  return all_nums.x;
}

void send_result(int res)
{
  printf("Sending Final Solution X = %d\n\n", res);

  char rez_str[20];
  sprintf(rez_str, "%d\n", res);
  serial_port.write(rez_str, strlen(rez_str)); // Send it!
}

int find_difficult_x()
{
  int a = 0, b = 0, c = 0;
  int ignore = 10; // This will be divider index

  int divider = 0;

  c = all_nums.x * -1; // Turn our free number to its original state

  for (int i = 0; i < all_nums.count; i++) // Go through our collection of simb, num
  {
    if (all_nums.simb[i] == '^') // We have quadratic equation
    {
      if (i != 0) // Is there a coef for a
      {
        a = all_nums.num[i - 1]; // Lets grab a coef
        if (i - 1 > 0)
          b += all_nums.num[i - 2]; // Additionaly lets grab b coef if it exists
      }
    }
    else if (all_nums.simb[i] == '/') //Check if divider exists
    {
      ignore = i;
      divider = all_nums.num[i];
      break;
    }
  }

  if (a == 0 && divider != 0) // Grab b coef if its not quadratic equation
    b += all_nums.num[ignore - 1];

  if (divider != 0) // Correct values if we had a divider
  {
    c *= divider;
    a *= divider;
    b *= divider;
    b++; // Divider/Divider = 1, lets add it to b
  }

  if (VERBOSE)
  {
    printf("a: %d\n", a);
    printf("b: %d\n", b);
    printf("c: %d\n", c);
  }

  if (a != 0) // This is quadratic equation
  {
    int x1 = ((-1 * b) + (sqrt(b * b - (4 * a * c)))) / (2 * a); // First solution
    int x2 = ((-1 * b) - (sqrt(b * b - (4 * a * c)))) / (2 * a); // Second solution

    if (VERBOSE)
    {
      printf("X1 Solution: %d\n", x1);
      printf("X2 Solution: %d\n", x2);
    }

    if (x1 > 0) // Lets pick the positive solution
      all_nums.x = x1;
    else
      all_nums.x = x2;
  }
  else
    all_nums.x = (-1 * c) / b; // This is not a quadratic equation

  return all_nums.x;
}