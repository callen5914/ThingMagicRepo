// This #include statement was automatically added by the Particle IDE.
#include <PublishManager.h>
#include <vector>

#define MAX_MESSAGE_LENGTH 32
#define THRESHOLD 5            // Min seconds to prevent double detection
#define PUSH_BUTTON_PIN  9     // push button connection (0 = disabled)
#define START_LED_PIN    8     // led to show that starttime has been recorded (0 = disabled)

///////////////////////////////////////////////////////////////
/// no configuration changes needed beyond this point       ///
///////////////////////////////////////////////////////////////

// some compilers expect the routines to be declared before encountering in the code
// not sure this is necessary with Particle compiler. if not comment or remove
void PublishRunnerStats (int i);
void add_new_runner(char * crossingRunner, uint32_t crossingTime, uint16_t rnd, bool pub);
bool checkForNewRunner(const char endMarker);
void disp_runner_stat();
void check_cmd();
void handle_cmd(char *cmd);

struct Runner {
    char runnerTag[32];         // save EPC
    uint32_t ts;                // crossing line timestamp
    uint8_t round;              // rounds counter
    uint32_t fastest_ts;        // fastest lap timestamp
    bool pb;                    // true : pending publish
};

std::vector<Runner> runners;
PublishManager<> publishManager;

char incomingMessage[MAX_MESSAGE_LENGTH];
bool enablePUB = true;          // if false NO publishing is performed

/* handle commands example
 *
 * #CLEAR!# : clear the list completely (EXTRA !)
 * #STOP#   : stop publishing
 * #START#  : start publishing
 * #RDY#    : it will clear runner statistics, wait for button to be pressed, set the start time for all runners and enable publishing
 * #DSP#    : display runner statistics
 * #ADD#    : add a new runner entry (removes if runner existed first)
 * #REMOVE# : remove a runner entry
 * #RESET#  : reset an existing runner entry to start
 * #RESETALL!# : resets all runner statistics (EXTRA !)
 * #UPDRND# : Update runner round statistics
 */

void handle_cmd(char *cmd)
{
    char *p;
    char buf[MAX_MESSAGE_LENGTH];

    // empty runner list
    if (strcmp(cmd, "#CLEAR!#") == 0) {
        runners.clear();
        Serial.printf("All runners have been removed\n");
    }
    // disable publish
    else if (strcmp(cmd, "#STOP#") == 0) {
        enablePUB = false;
        Serial.printf("Publishing has been stopped\n");
        if (START_LED_PIN > 0) digitalWrite(START_LED_PIN, LOW);
    }
    // enable publish
    else if (strcmp(cmd, "#START#") == 0) {
        enablePUB = true;
        Serial.printf("Publishing has been enabled\n");
    }

    // It will reset/clear all runner statistics,
    // wait for button to be pressed,
    // set the start time for all runners,
    // set the start LED and enable publishing
    else if (strcmp(cmd, "#RDY#") == 0)
    {
        handle_cmd("#RESETALL!#");

        if (PUSH_BUTTON_PIN > 0)
            while(digitalRead(PUSH_BUTTON_PIN) == HIGH); // wait for button pressed

        // set runners start time
        uint32_t StartTime = Time.now();
        for (size_t i = 0; i < runners.size(); i++)  runners[i].ts = StartTime;

        enablePUB = true;
        Serial.printf("Started\n");
        if (START_LED_PIN > 0) digitalWrite(START_LED_PIN, HIGH);
    }

    // display runner statistics
    else if (strcmp(cmd, "#DSP#") == 0)   disp_runner_stat();

    // remove all runner statistics
    else if (strcmp(cmd, "#RESETALL!#") == 0)
    {
        for (size_t i = 0; i < runners.size(); i++)
        {
            runners[i].ts = 0;
            runners[i].round = 0;
            runners[i].fastest_ts = 0;
            runners[i].pb = false;
        }

        Serial.printf("All runner statistics have been reset\n");
     }

    // initialize new runner and tries to remove first.
    // #ADD# followed by epc : #ADD#E2000015860E028815408029
    else if ((p = strstr(cmd, "#ADD#")) > 0)  {
        p  += 5;    // skip #ADD#

        sprintf(buf,"#REMOVE#%s", p);
        handle_cmd(buf);

        add_new_runner(p,0,0,false);
        Serial.printf("Runner %s has been added\n", p);
    }

    // remove runner : #REMOVE# followed by EPC : #REMOVE#E2000015860E028815408029
    else if ((p = strstr(cmd, "#REMOVE#")) > 0) {
        p  += 8;    // skip #REMOVE#

        for (size_t i = 0; i < runners.size(); i++)
        {
            if (strcmp(runners[i].runnerTag, p) == 0)
            {
                runners.erase(runners.begin() + i);
                Serial.printf("Runner %s has been removed\n", p);
                return;
            }
        }
    }

    // clear runner statistics : #RESET# followed by EPC: #RESET#E2000015860E028815408029
    else if ((p =strstr(cmd, "#RESET#")) > 0)  {

        p  += 7;    // skip #RESET#

        for (size_t i = 0; i < runners.size(); i++)
        {
            if (strcmp(runners[i].runnerTag, p) == 0)
            {
                runners[i].ts = 0;
                runners[i].round = 0;
                runners[i].fastest_ts = 0;
                runners[i].pb = false;
                Serial.printf("Runner %s statistics are reset\n", p);
                return;
            }
        }
    }
    // update runner round statistics : #UPDRND# followed by EPC?X
    // #UPDRND#E2000015860E028815408029?12 (12rounds)
    else if ((p =strstr(cmd, "#UPDRND#"))> 0)  {

        p  += 8;    // skip #UPDRND#

        // obtain runnerTag
        for(size_t j = 0; j < sizeof(buf) ; j++)
        {
            if (*p == '?')  break;
            buf[j] = *p++;
            buf[j+1] = 0x0;
        }

        // find runner
        for (size_t i = 0; i < runners.size(); i++)
        {
            if (strcmp(runners[i].runnerTag, buf) == 0)
            {
                runners[i].round = (uint8_t) atoi(++p);
                Serial.printf("Runner %s rounds are updated to %d\n", buf, atoi(p));
                break;
            }
        }
    }
    else
    {
        Serial.printf("Unknown command %s. Ignored.\n", cmd);
    }
}
/* read commands from the keyboard
 * if \n was found it will call handle_cmd() to execute */
void check_cmd()
{
  static char message[32];
  char c;
  static int  i = 0;

  while (Serial.available())
  {
    c = Serial.read();

    if (c == '\n')
    {
        message[i] = 0x0;       // terminate
        handle_cmd(message);

        i = 0;                  // reset
        return;
    }
    else
    {
        message[i++] = c;

        if (i > 31) {
            Serial.println("message to long. Removed");
            i = 0;
        }
    }
  }
}


void setup() {
    Serial.begin(9600);  //<<<<< don't forget!
    Serial1.begin(115200);

    if (PUSH_BUTTON_PIN > 0)
    {
        // connect a pushbutton between the pin and ground
        pinMode(PUSH_BUTTON_PIN,INPUT, INPUT_PULLUP);
    }

    if (START_LED_PIN > 0)
    {
        // connect a led between the pin, 4k7 resistor and ground
        pinMode(START_LED_PIN,OUTPUT);
        digitalWrite(START_LED_PIN, LOW);
    }

}

void loop() {
    publishManager.process();

    char* crossingRunner = incomingMessage;

    // read local keyboard
    check_cmd();

    if (checkForNewRunner('\n')) {

        Serial.printf("%s just crossed the line...", crossingRunner);
        uint32_t crossingTime = Time.now();
        bool Notfound = true;

        for (size_t i = 0; i < runners.size(); i++) {

            if (strcmp(runners[i].runnerTag, crossingRunner) == 0) {

                // Prevent incorrect double detection on the crossing line
                if (crossingTime - runners[i].ts > THRESHOLD)
                {
                    // a valid time detected, was it pending publishing?
                    if (runners[i].pb)  PublishRunnerStats(i);

                    runners[i].round += 1;           // count round

                    Serial.printf("finishing round %d "), runners[i].round;

                    // in case the runner was initialized before with a command and NOT started with #RDY# command
                    // the first round just ended, then there was no previous time was recorded so we do not know round time.
                    if (runners[i].ts != 0)
                        Serial.printf("in %d seconds. "), crossingTime - runners[i].ts);

                    // if no fastest lap recorded yet
                    if (runners[i].fastest_ts == 0)
                    {
                         // in case the runner was initialized before with a command and NOT started with #RDY# command
                         // the first round just ended, then there was no previous time was recorded before so we do not
                         // know round time, else this was the second round.
                         if (runners[i].ts != 0) runners[i].fastest_ts = crossingTime - runners[i].ts;
                         Serial.printf(".\n");
                    }
                    else
                    {
                        // check if fastest lap
                        if (runners[i].fastest_ts > crossingTime - runners[i].ts)
                        {
                            Serial.printf("The fastest round !! %d seconds faster than previous\n").
                            crossingTime - runners[i].ts - runners[i].fastest_ts;

                            runners[i].fastest_ts = crossingTime - runners[i].ts
                        }
                        else
                        {
                            Serial.printf("This is %d S slower than fastest of %d S\n"),
                             crossingTime - runners[i].ts - runners[i].fastest_ts, runners[i].fastest_ts ;
                        }
                    }

                    runners[i].ts = crossingTime;
                    runners[i].pb = true;            // needs to be published
                }

                Notfound = false;                   // we did find the runner
                break;                              // no need to keep searching
            }
        }

        // add the runner, auto-detect after first round
        if (Notfound)
        {
            // runner EPC, crossing time, 1 = first round, true = publish
            add_new_runner(crossingRunner, crossingTime, 1, true);
        }
    }

    /* look for entry that has not been published yet */
    if (runners.size())
    {
        for (size_t i = 0; i < runners.size(); i++)
        {
            if (runners[i].pb)          // waiting to publish
            {
                PublishRunnerStats(i);
                // a break; could be included here if you only want to publish max 1 every loop
            }
        }
    }
}

/* add a new runner to the list
 * crossingRunner = EPC
 * crossingTime = last crossingtime or 0 (in case of initialize)
 * rnd = starting round. 0 (in case of initialize) or 1 (in case of auto-detect)
 * pub = publish info. false (in case of initialize) or true (in case of auto-detect)
 */
void add_new_runner(char * crossingRunner, uint32_t crossingTime, uint16_t rnd, bool pub)
{
    Runner newRunner;

    strncpy(newRunner.runnerTag, crossingRunner, sizeof(newRunner.runnerTag));
    newRunner.ts = crossingTime;
    newRunner.round = rnd;
    newRunner.fastest_ts = 0;
    newRunner.pb = pub;

    runners.push_back(newRunner);     // append to end
    Serial.printf("added runner %s.\n", crossingRunner);
}

/* Publish one entry from the list */
void PublishRunnerStats (int i)
{
    char pubMessage[64];

    // if publishing is not enabled
    if (!enablePUB ) return;

    sprintf(pubMessage, "{\"tag\":\"%s\",\"time\":%d}", runners[i].runnerTag, runners[i].ts);
    publishManager.publish("Lap", pubMessage);
    Serial.printf("%s sent to publishManager with a time of: %d\n.", runners[i].runnerTag , runners[i].ts);
    runners[i].pb = false;  // indicated published
}

/* display runner statistics on the screen connected to Particle */
void disp_runner_stat()
{
    for (size_t i = 0; i < runners.size(); i++) {
       Serial.printf("Runner %s, Rounds %d, Fastest lap %d\n", runners[i].runnerTag , runners[i].round, runners[i].fastest_ts);
    }
}

/* Check for new RFID tag
 * return :
 *  true if content received and terminated   (stored in global incomingMessage)
 *  false if no (extra) content received
 */
bool checkForNewRunner(const char endMarker) {

  static byte idx = 0;
  if (Serial1.available()) {
    incomingMessage[idx] = Serial1.read();
    if (incomingMessage[idx] == endMarker) {
      incomingMessage[idx] = '\0';
      idx = 0;
      return true;
    } else {
      idx++;
      if (idx > MAX_MESSAGE_LENGTH - 1) {
        //stream.print(F("{\"error\":\"message too long\"}\n"));  //you can send an error to sender here
        idx = 0;
        incomingMessage[idx] = '\0';
      }
    }
  }
  return false;
}
