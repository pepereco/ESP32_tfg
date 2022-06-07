
//secuencia media fase
const int numSteps = 8;
const int stepsLookup[8] = { B1000, B1100, B0100, B0110, B0010, B0011, B0001, B1001 };

//Params horizontal stepper
int motorSpeedHoriz = 1200;   //variable para fijar la velocidad

int stepsPerRev = 4076;  // pasos para una vuelta completa

int stepCounter_horiz = 0;     // contador para los pasos

void stepper_horizontal_setup()
{
  //declarar pines como salida
  pinMode(stepper_5v_pin1 , OUTPUT);
  pinMode(stepper_5v_pin2, OUTPUT);
  pinMode(stepper_5v_pin3, OUTPUT);
  pinMode(stepper_5v_pin4, OUTPUT);
}


void back_horiz(int steps)
{
  for (int i = 0; i < steps; i++)
  {
    current_step_horizontal-=1;
    stepCounter_horiz++;
    if (stepCounter_horiz >= numSteps) stepCounter_horiz = 0;
    setOutput(stepCounter_horiz);
    delayMicroseconds(motorSpeedHoriz);
  }
  
}

void front_horiz(int steps)
{
  for (int i = 0; i < steps; i++)
  {
    stepCounter_horiz--;
    current_step_horizontal+=1;
    if (stepCounter_horiz < 0) stepCounter_horiz = numSteps - 1;
    setOutput(stepCounter_horiz);
    delayMicroseconds(motorSpeedHoriz);
  }
  
}

void start_horiz(){
  //Starting position, by doing 10 revs back
  back_horiz(40760);
  stepCounter_horiz=0;
  current_step_horizontal=0;
}

void setOutput(int step)
{
  digitalWrite(stepper_5v_pin1 , bitRead(stepsLookup[step], 0));
  digitalWrite(stepper_5v_pin2, bitRead(stepsLookup[step], 1));
  digitalWrite(stepper_5v_pin3, bitRead(stepsLookup[step], 2));
  digitalWrite(stepper_5v_pin4, bitRead(stepsLookup[step], 3));
}
