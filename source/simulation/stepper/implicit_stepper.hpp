#ifndef IMPLICIT_STEPPER_HPP
#define IMPLICIT_STEPPER_HPP

#include "general_definitions.hpp"
#include "utilities/almost_equal.hpp"
#include "preprocessor/input_parameters.hpp"

/**
 * Explicit Strong Stability preserving Runge-Kutta methods
 * Class discretizes an ODE using strong stability preserving Runge Kutta methods.
 */
class ImplicitStepper : public Stepper {
  private:
    uint order;
    uint nstages;
    double dt;
    double theta;

    uint step;
    uint stage;
    uint timestamp;

    double t;
    double ramp_duration;
    double ramp;

  public:
    ImplicitStepper() = default;
    ImplicitStepper(const StepperInput& stepper_input)
        : order(stepper_input.order),
          nstages(stepper_input.nstages),
          dt(stepper_input.dt),
          step(0),
          stage(0),
          timestamp(0),
          t(0.),
          ramp_duration(stepper_input.ramp_duration),
          ramp(Utilities::almost_equal(ramp_duration, 0) ? 1. : 0.) {
        if (this->order == 1 && this->nstages == 1) {
            this->theta = 0.0;
        } else if (this->order == 2 && this->nstages == 1) {
            this->theta = 0.5;
        } else if (this->order == 2 && this->nstages == 2) {
            this->nstages = 1;
            this->theta   = 0.5;
        } else {
            std::cout << this->order << ' ' << this->nstages << '\n';
            throw std::logic_error("Fatal Error: invalid implicit method entered!");
        }
    }

    uint GetOrder() const { return this->order; }
    uint GetNumStages() const { return this->nstages; }
    double GetDT() const { return this->dt; }
    double GetTheta() const { return this->theta; }

    void SetDT(double dt) { this->dt = dt; };

    uint GetStep() const { return this->step; }
    uint GetStage() const { return this->stage; }
    uint GetTimestamp() const { return this->timestamp; }

    double GetTimeAtCurrentStage() const { return this->t; }
    double GetRamp() const { return this->ramp; }

    ImplicitStepper& operator++() {
        ++(this->stage);
        ++(this->timestamp);

        this->stage = this->stage % this->nstages;

        if (this->stage == 0) {
            this->t += this->dt;
            ++(this->step);
        }

        if (!Utilities::almost_equal(this->ramp_duration, 0)) {
            this->ramp = std::tanh(2 * (this->GetTimeAtCurrentStage() / 86400) / this->ramp_duration);
        }

        return *this;
    }
};

#endif