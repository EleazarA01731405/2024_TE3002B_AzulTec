#include <micro_ros_arduino.h>

#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/float32.h>

//Publishers
rcl_publisher_t publisher_raw_pot;
rcl_publisher_t publisher_voltage;

//Subscriber
rcl_subscription_t direcrion_motor, duty_cycle_subs;

//msg type int
std_msgs__msg__Int32 dir;

//msg type float
std_msgs__msg__Float32 raw_pot, voltage, duty_cycle;

//general handling
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;

//doe def
rcl_node_t node_handle;
rcl_timer_t timer10, timer100;


#define LED_PIN 13 // ERROR
#define LED2_PIN 15 // PWM
#define POT_PIN 14 // POTENTIOMETER

//global variables
int pot = 0;
int pwm = 0;

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

void error_loop(){
  while(1){
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}

void timer10_callback(rcl_timer_t * timer, int64_t last_call_time)
{  
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    pot = analogRead(POT_PIN)// Corrected to read the potentiometer
  }
}

void timer100_callback2(rcl_timer_t * timer2, int64_t last_call_time)
{  
  RCLC_UNUSED(last_call_time);
  if (timer2 != NULL) { // Corrected the timer variable check
    raw_pot.data = pot;
    voltage.data = pot*3.3/4160
    RCSOFTCHECK(rcl_publish(&publisher_raw_pot, &raw_pot, NULL)); // Updated to publish Float32 message
    
    RCSOFTCHECK(rcl_publish(&publisher_voltage, &voltage, NULL));
  }
}

void subscription_pwm_callback(const void * msgin)
{
  const std_msgs__msg__Float32 * duty_cycle = (const std_msgs__msg__Float32 *)msgin; 
  pwm = (duty_cycle->data)*255; //El valor de entrada va de 0 a 1, mapeamos
  ledcWrite(PWM1_Ch, pwm); //Escribimos sobre el canal de pwm la velocidad deseada
}

void subscription_direction(const void * msgin)
{
  const std_msgs__msg__Int32 * dir = (const std_msgs__msg__Int32 *)msgin; 
  dir_ = dir->data; //Obtenemos el valor y lo guardamos en dir_

  //Realizamos las comparaciones
  if(dir_ == 1){
    digitalWrite(In1, LOW);   //  1: Derecha/Izquierda
    digitalWrite(In2, HIGH);  // -1: Izquierda/Derecha
  }                           //  0: Se detiene

  else if (dir_ == -1){
    digitalWrite(In1, HIGH); 
    digitalWrite(In2, LOW);
  }

  else if (dir_ == 0){
    digitalWrite(In1, LOW); 
    digitalWrite(In2, LOW);
  }
}

void setup() {
  set_microros_transports();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);   
  
  delay(2000);

  allocator = rcl_get_default_allocator();

  //create init_options
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, "Timer_1", "", &support));

  // create node
  RCCHECK(rclc_node_init_default(&node2, "Timer_2", "", &support));

  // create publisher
  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "Timer_1"));
    
  // create publisher
  RCCHECK(rclc_publisher_init_default(
    &publisher2,
    &node2,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, fmsg, Float32),
    "Timer_2"));

  // create timer,
  const unsigned int timer_timeout = 10;
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  // create timer,
  const unsigned int timer2_timeout = 100;
  RCCHECK(rclc_timer_init_default(
    &timer2,
    &support,
    RCL_MS_TO_NS(timer2_timeout),
    timer_callback2));

  RCCHECK(rclc_subscription_init_default(
    &direction_motor, //Handle a subscription 
    &node_handle, //Handle al nodo 
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), //Tipo de dato
    "micro_ros_esp32/direction")); 

  // create executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));
  RCCHECK(rclc_executor_add_timer(&executor, &timer2));

  voltage.data = 0; 
  raw_pot.data = 0; 
}

void loop() {
  delay(100);
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
}
