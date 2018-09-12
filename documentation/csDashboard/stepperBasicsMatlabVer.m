%% intro to syringe pump calculations
% basic linear transmission and notes on specific details. 
%
%
%
% cdeister@brown.edu
% 6/1/2018
%
%
%% a key variable is the amount of threads per mm your rod has "pitch"
%
%
% I've been opting for very standard single threaded rods as opposed to the
% elaborate multi-start rods popular on 3d printers.
%
% I have been using M8-1.25 rods, that means 8mm in diameter and 1.25 mm
% per turn. I think M8-1.0 would be better, but 1.25 works well.

rod.pitch = 1.25;

%% other key variables are diameter and length of syringe
%
% i looked up some standards (in mm)
syringe_10ml.diameter = 15.9;
syringe_10ml.length = 85.3;
syringe_10ml.volume = syringe_10ml.length*((syringe_10ml.diameter/2)^2);
%
% we are going to translate linear motion into fluid transmission
% so we need to subdivide our liquid volume into the amount we would get
% for each mm of push on the cylinder
syringe_10ml.volumePerMM = syringe_10ml.volume/syringe_10ml.length;
%% now we deal with motor variables
%
% stepper motors come as 200 or 400 steps/rev (1.8 or 0.9 degrees per step
% respectivley)
%
motor.highRes_stepsPerRevoultion = 400;
motor.lowRes_stepsPerRevoultion = 200;

% microstepping.
% why microstep? we want to be able to make tiny, but still precise, steps
% to deliver real small volumes of liquid etc. if we just take the 400
% steps the motor gives us we get:
%
% we know the amount of liquid we dispense each time we push a mm on the
% syringe (63.2025 ml for a 10 ml syringe) the variable above is
% "syringe_10ml.volumePerMM" so we just divide that by the step number
% (without microsteps) our chosen motor has:
%
dispense.volPerRevolution = syringe_10ml.volumePerMM * rod.pitch
dispense.volPerStep = dispense.volPerRevolution/motor.highRes_stepsPerRevoultion
%
% for a 10 ml syringe and a 400 step motor, we get 0.1975 ml (198 ul) per
% full step. This is too much to give for a reward. 
%
% we could get a finer rod, M8 specs down to 0.75 pitch:
%
dispense.volPerRevolution_fine = syringe_10ml.volumePerMM * 0.75
dispense.volPerStep_fine = dispense.volPerRevolution_fine/motor.highRes_stepsPerRevoultion
%
% This gives 119 ul per step, which isn't that much better.
% ~~~~~~
% ~~~~~~~~ mic ~~~
% ~~~~~~~~~~~~ ro ~~~
% ~~~~~~~~~~~~~~~~ stepping ~~~
%
% let's go back to our actual rod pitch of 1.25 etc. and see what
% microstepping does. 
%
% in general the lowest fraction of step you should be able to go down to
% and maintain consistent torque is 1/32 or 1/64th of a full step with
% these drivers. But, you can try 1/128, but depends on the motor. When I
% measured 1/128 the fraction delivered is less than 80% of what is
% intended, which is on par with solenoids. However, 1/32 and 1/64 will
% give >90% acuracy even with junk plastic syringes. 

motor.microstepResolution = 256;
motor.microstepFactor_large = 32; % 1/microstepFactor is how much we divide a microstep.

% we scale our original volume per step by the microstepping resolution
dispense.volPerMicrostep = dispense.volPerRevolution/(motor.highRes_stepsPerRevoultion*motor.microstepResolution);
%
% this gives 0.0007715 ml per microstep; or 0.77 ul per microstep
% if we want consistent torque we should avoid going lower than 256/32
%
dispense.volPerIndBolus_large = dispense.volPerMicrostep * (motor.microstepResolution/motor.microstepFactor_large)
%
% this gives 0.0062 ml per bolus or 6.2 ul 
% 
% anecdotally, I measured ~99% delivery over many trials down to 1/64th
% at that point you would get twice as small ~ 3.1 ul
motor.microstepFactor_norm = 64;
dispense.volPerIndBolus_norm = dispense.volPerMicrostep * (motor.microstepResolution/motor.microstepFactor_norm)
%
%
% based on current settings, the number of microsteps per bolus
% we multiply the actual steps, by the microstepping resolution, then
% divide by the factor (1/32, 1/64 etc.)
dispense.stepsPerBolus_large = (motor.highRes_stepsPerRevoultion * ...
    motor.microstepResolution)/motor.microstepFactor_large

dispense.stepsPerBolus_norm = (motor.highRes_stepsPerRevoultion * ...
    motor.microstepResolution)/motor.microstepFactor_norm

% note that if you went to 1/128th stepping, you would get ~1.6 ul
% for this, you would set the steps/bolus to 800

% the other benifit to microstepping is that we can create acceleration and
% speed profiles to help shape deleivery. conceptually, backlash of the
% syringe can be calculated and compensated for like a 3d printer does. 

% currently csDashboard is set with 1600 microsteps as a syringe bolus,
% assuming a 400 step motor will give 3.1 ul. But, moving steps up and down
% with the button will move the steps in 800 step incrments, so you can do
% 1/128 stepping and get 1.6 ul.

% also, note from above a finer pitch rod and lead screw (nut) can lead to
% an up to 40% improvment in resolution, but at some point we lose torque.
% regardless, if we swapped out to 0.75 then we would get a default drop of
% 1.9 ul and a tiny drop of 0.96 ul
% it is harder to find 0.75 pitch than 1 or 1.25

