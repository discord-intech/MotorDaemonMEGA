
#include <lib/parson.h>
#include "MotionController.hpp"
#include "lib/LinkedResponseList.h"

#define RESULT_CODE_1 19
#define RESULT_CODE_2 20


MotionController* motion;

LinkedResponseList* responses = nullptr;

char *buffer;
volatile unsigned short pos = 0;

void write_responses()
{
    LinkedResponseList* current = responses;

    while (current)
    {
        Serial.write((uint8_t)RESULT_CODE_1);
        Serial.write((uint8_t)RESULT_CODE_2);
        Serial.print(current->data);
        for(int i = 0 ; i<5 ; i++) Serial.write((uint8_t)13);
        current = current->next;
    }
}

void motionControllerWrapper()
{
    static unsigned int count = 0;

    motion->mainHandler();

    if(count++ % 100 == 0)
    {
        write_responses();
    }
}

void orderHandler()
{
    char* order = strtok(buffer, " ");

    char content[200];
    memset(&content, 0, 200);
    int resultCode = 0;

    if(!strcmp(order, "d"))
    {
        char* d = strtok(nullptr, " ");
        motion->orderTranslation(strtol(d, nullptr, 10));
    }
    else if(!strcmp(order, "ack"))
    {
        unsigned int id = static_cast<unsigned int>(strtol(strtok(nullptr, " "), nullptr, 10));

        if(responses)
        {
            LinkedResponseList* current = responses;
            LinkedResponseList* precedent = nullptr;

            while (current)
            {
                if(id == current->resultID)
                {
                    if(precedent)
                    {
                        precedent->next = current->next;
                    }
                    else
                    {
                        responses = current->next;
                    }

                    json_free_serialized_string(current->data);
                    current->next = nullptr;
                    free(current);
                    return;
                }
                precedent = current;
                current = current->next;
            }
        }

    }
    else if(!strcmp(order, "stop"))
    {
        motion->stop();
    }
    else if(!strcmp(order, "neon"))
    {
        char* speed = strtok(nullptr, " ");
        motion->setNeonSpeed(static_cast<unsigned char>(strtol(speed, NULL, 10)));
    }
    else if(!strcmp(order, "cr"))
    {
        char* d = strtok(nullptr, " ");
        motion->orderCurveRadius(strtol(d, nullptr, 10));
    }
    else if(!strcmp(order, "setspeed"))
    {
        char* s = strtok(nullptr, " ");
        motion->setSpeedTranslation(strtol(s, nullptr, 10));
    }
    else if(!strcmp(order, "traj")) // TODO Check this method, very experimental
    {
        char * info = strtok(nullptr, ";");
        Trajectory trajectory(static_cast<unsigned short>(strtol(strtok(info, ","), nullptr, 10)),
                              strtol(strtok(nullptr, ","), nullptr, 10));


        unsigned int delta = strcspn(buffer, ";");
        char* point = strtok(&buffer[delta], ";");

        while(point != nullptr)
        {
            int dist = strtol(strtok(point, ","), nullptr, 10);
            Cinematic* p = new Cinematic(ABS(dist),
                                         strtol(strtok(nullptr, ","), nullptr, 10), dist > 0);
            trajectory.setPoint(p);
            delta += strcspn(&buffer[delta], ";");
            point = strtok(&buffer[delta], ";");
        }

        trajectory.seek(0);
        motion->setTrajectory(&trajectory);
    }
    else if(!strcmp(order, "testspeed"))
    {
        char* s = strtok(nullptr, " ");
        motion->testSpeed(strtol(s, nullptr, 10));
    }
    else if(!strcmp(order, "testpos"))
    {
        char* d = strtok(nullptr, " ");
        motion->testPosition(strtol(d, nullptr, 10));
    }
    else if(!strcmp(order, "setangle"))
    {
        char* a = strtok(nullptr, " ");
        motion->setAngle(strtod(a, nullptr));
    }
    else if(!strcmp(order, "setpos"))
    {
        char* x = strtok(nullptr, " ");
        char* y = strtok(nullptr, " ");
        motion->setPosition(strtod(x, nullptr), strtod(y, nullptr));
    }
    else if(!strcmp(order, "setConsts"))
    {
        motion->setLeftSpeedTunings(static_cast<float>(strtod(strtok(nullptr, " "), nullptr)),
                                   static_cast<float>(strtod(strtok(nullptr, " "), nullptr)),
                                   static_cast<float>(strtod(strtok(nullptr, " "), nullptr)));
        motion->setRightSpeedTunings(static_cast<float>(strtod(strtok(nullptr, " "), nullptr)),
                                   static_cast<float>(strtod(strtok(nullptr, " "), nullptr)),
                                   static_cast<float>(strtod(strtok(nullptr, " "), nullptr)));
        motion->setTranslationTunings(static_cast<float>(strtod(strtok(nullptr, " "), nullptr)),
                                   static_cast<float>(strtod(strtok(nullptr, " "), nullptr)),
                                   static_cast<float>(strtod(strtok(nullptr, " "), nullptr)));
        motion->setCurveTunings(    static_cast<float>(strtod(strtok(nullptr, " "), nullptr)),
                                   static_cast<float>(strtod(strtok(nullptr, " "), nullptr)),
                                   static_cast<float>(strtod(strtok(nullptr, " "), nullptr)));
    }
    else
    {
        resultCode = 1;
        snprintf(content, 200, "Bad order : %s", order);
    }

    if(resultCode == 0)
    {
        snprintf(content, 200, "Command executed successfully %s", order);
    }

    free(buffer);
    buffer = (char*)malloc(256 * sizeof(char));
    memset(buffer, 0, 256);
    pos = 0;
    unsigned int resID = responses ? responses->resultID + 1 : 0;
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_number(root_object, "code", resultCode);
    json_object_set_number(root_object, "id", resID);
    json_object_set_string(root_object, "content", (const char*)&content);

    char *serialized_string = json_serialize_to_string(root_value);

    LinkedResponseList* res = static_cast<LinkedResponseList *>(malloc(sizeof(LinkedResponseList)));
    res->resultID = resID;
    res->data = serialized_string;
    res->next = responses;

//    Serial.write((uint8_t)RESULT_CODE_1);
//    Serial.write((uint8_t)RESULT_CODE_2);
//    Serial.print(serialized_string);
//    for(int i = 0 ; i<5 ; i++) Serial.write((uint8_t)13);
//
//    json_free_serialized_string(serialized_string);
    json_value_free(root_value);

    write_responses();
}

void setup()
{
    Serial.begin(115200);

    motion = new MotionController();

    motion->init();

    responses = static_cast<LinkedResponseList *>(malloc(sizeof(LinkedResponseList)));
    responses->resultID = 0;

    //Timer1.attachInterrupt(motionControllerWrapper).setFrequency(1000).start();

    Timer3.initialize(1000);
    Timer3.attachInterrupt(motionControllerWrapper);

    buffer = (char*)malloc(256 * sizeof(char));
    memset(buffer, 0, 256);

    while(Serial.available() > 0) Serial.read(); // Cleaning the input buffer

}

void loop()
{

    if(Serial.available() > 0)
    {
        int c = Serial.read();

        //Serial.println('a');
        
        if(c == 13)
        {
            buffer[pos++] = '\0';
            orderHandler();
        }
        else
        {
            buffer[pos++] = static_cast<char>(c);
        }

    }

}
