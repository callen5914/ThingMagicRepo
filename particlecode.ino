// This #include statement was automatically added by the Particle IDE.
#include <PublishManager.h>
#include <vector>

constexpr size_t MAX_MESSAGE_LENGTH = 32;
constexpr uint16_t CROSSING_HYSTERESIS_SECONDS = 3;

PublishManager<> publishManager;

const unsigned long interval = 8000;

struct Runner {
    char runnerTag[32];
    uint32_t ts;
};

std::vector<Runner> runners;

const char* knownRunners[] = {"TestTrackTag01", "TestTrackTag02", "TestTrackTag03", "TestTrackTag04"};

void setup() {
    Serial.begin(9600);  //<<<<< don't forget!
    Serial1.begin(115200);
}

void loop() {
    publishManager.process();
    if (const char* crossingRunner = checkForNewRunner('\n')) {
        Serial.printf("%s just crossed the line...", crossingRunner);
        uint32_t crossingTime = Time.now();
        bool isKnownRunner = false;
        for (auto& t : knownRunners) {
            if (strcmp(crossingRunner, t) == 0) {
                isKnownRunner = true;
                Serial.printf("%s is known to me", crossingRunner);
            }
        }
        if (isKnownRunner) {
            Runner newRunner;
            strcpy(newRunner.runnerTag, crossingRunner);
            newRunner.ts = crossingTime;
            for (size_t i = 0; i < runners.size(); i++) {
                if (strcmp(runners[i].runnerTag, newRunner.runnerTag) == 0) {
                    runners.erase(runners.begin() + i);
                    Serial.printf("%s was in the queue.", crossingRunner);
                }
            }
            runners.insert(runners.begin(), newRunner);
            Serial.printf("%s added to the queue.", crossingRunner);
        }
    }
    
    if (runners.size()) {
        if (Time.now() - runners.back().ts > CROSSING_HYSTERESIS_SECONDS) {
            char pubMessage[64];
            sprintf(pubMessage, "{\"tag\":\"%s\",\"time\":%d}", runners.back().runnerTag, runners.back().ts);
            publishManager.publish("Lap", pubMessage);
            Serial.printf("%s sent to publishManager with a time of: %d.", runners.back().runnerTag , runners.back().ts);
            runners.pop_back();
        }
    }
    
}

const char* checkForNewRunner(const char endMarker) {
  static char incomingMessage[MAX_MESSAGE_LENGTH] = "";
  static byte idx = 0;
  if (Serial1.available()) {
    incomingMessage[idx] = Serial1.read();
    if (incomingMessage[idx] == endMarker) {
      incomingMessage[idx] = '\0';
      idx = 0;
      return incomingMessage;
    } else {
      idx++;
      if (idx > MAX_MESSAGE_LENGTH - 1) {
        //stream.print(F("{\"error\":\"message too long\"}\n"));  //you can send an error to sender here
        idx = 0;
        incomingMessage[idx] = '\0';
      }
    }
  }
  return nullptr;
}
