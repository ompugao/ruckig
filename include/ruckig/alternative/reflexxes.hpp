#pragma once

#include <chrono>
#include <memory>

#include <ReflexxesAPI.h>
#include <RMLPositionFlags.h>
#include <RMLPositionInputParameters.h>
#include <RMLPositionOutputParameters.h>

#include <ruckig/parameter.hpp>


namespace ruckig {

template<size_t DOFs>
class Reflexxes {
    InputParameter<DOFs> current_input;
    std::shared_ptr<ReflexxesAPI> rml;
    std::shared_ptr<RMLPositionInputParameters> input_parameters;
    std::shared_ptr<RMLPositionOutputParameters> output_parameters;
    std::shared_ptr<RMLVelocityInputParameters> input_vel_parameters;
    std::shared_ptr<RMLVelocityOutputParameters> output_vel_parameters;

    int result_value {0};
    RMLPositionFlags flags;
    RMLVelocityFlags vel_flags;

public:
    double delta_time;

    explicit Reflexxes(double delta_time): delta_time(delta_time) {
        rml = std::make_shared<ReflexxesAPI>(DOFs, delta_time);
        input_parameters = std::make_shared<RMLPositionInputParameters>(DOFs);
        output_parameters = std::make_shared<RMLPositionOutputParameters>(DOFs);
        input_vel_parameters = std::make_shared<RMLVelocityInputParameters>(DOFs);
        output_vel_parameters = std::make_shared<RMLVelocityOutputParameters>(DOFs);

        flags.SynchronizationBehavior = RMLPositionFlags::PHASE_SYNCHRONIZATION_IF_POSSIBLE;
        vel_flags.SynchronizationBehavior = RMLVelocityFlags::PHASE_SYNCHRONIZATION_IF_POSSIBLE;
    }

    Result update(const InputParameter<DOFs>& input, OutputParameter<DOFs>& output) {
        auto start = std::chrono::high_resolution_clock::now();

        if (input != current_input) {
            current_input = input;

            for (size_t dof = 0; dof < DOFs; ++dof) {
                if (input.target_acceleration[dof] != 0.0) {
                    return Result::Error;
                }
            }

            switch (input.type) {
            case InputParameter<DOFs>::Type::Position: {
                if (input.minimum_duration) {
                    input_parameters->SetMinimumSynchronizationTime(input.minimum_duration.value());
                }

                input_parameters->SetSelectionVector(input.enabled.data());
                input_parameters->SetCurrentPositionVector(input.current_position.data());
                input_parameters->SetCurrentVelocityVector(input.current_velocity.data());
                input_parameters->SetCurrentAccelerationVector(input.current_acceleration.data());
                input_parameters->SetTargetPositionVector(input.target_position.data());
                input_parameters->SetTargetVelocityVector(input.target_velocity.data());
                input_parameters->SetMaxVelocityVector(input.max_velocity.data());
                input_parameters->SetMaxAccelerationVector(input.max_acceleration.data());
                input_parameters->SetMaxJerkVector(input.max_jerk.data());
            } break;
            case InputParameter<DOFs>::Type::Velocity: {
                if (input.minimum_duration) {
                    input_vel_parameters->SetMinimumSynchronizationTime(input.minimum_duration.value());
                }

                input_vel_parameters->SetSelectionVector(input.enabled.data());
                input_vel_parameters->SetCurrentPositionVector(input.current_position.data());
                input_vel_parameters->SetCurrentVelocityVector(input.current_velocity.data());
                input_vel_parameters->SetCurrentAccelerationVector(input.current_acceleration.data());
                input_vel_parameters->SetTargetVelocityVector(input.target_velocity.data());
                input_vel_parameters->SetMaxAccelerationVector(input.max_acceleration.data());
                input_vel_parameters->SetMaxJerkVector(input.max_jerk.data());
            } break;
            }
        }

        switch (input.type) {
        case InputParameter<DOFs>::Type::Position: {
            result_value = rml->RMLPosition(*input_parameters, output_parameters.get(), flags);

            for (size_t dof = 0; dof < DOFs; ++dof) {
                output.new_position[dof] = output_parameters->NewPositionVector->VecData[dof];
                output.new_velocity[dof] = output_parameters->NewVelocityVector->VecData[dof];
                output.new_acceleration[dof] = output_parameters->NewAccelerationVector->VecData[dof];
            }
            output.duration = output_parameters->GetSynchronizationTime();
            output.new_calculation = output_parameters->WasACompleteComputationPerformedDuringTheLastCycle();
        } break;
        case InputParameter<DOFs>::Type::Velocity: {
            result_value = rml->RMLVelocity(*input_vel_parameters, output_vel_parameters.get(), vel_flags);

            for (size_t dof = 0; dof < DOFs; ++dof) {
                output.new_position[dof] = output_vel_parameters->NewPositionVector->VecData[dof];
                output.new_velocity[dof] = output_vel_parameters->NewVelocityVector->VecData[dof];
                output.new_acceleration[dof] = output_vel_parameters->NewAccelerationVector->VecData[dof];
            }
            output.duration = output_vel_parameters->GetSynchronizationTime();
            output.new_calculation = output_vel_parameters->WasACompleteComputationPerformedDuringTheLastCycle();
        } break;
        }

        auto stop = std::chrono::high_resolution_clock::now();
        output.calculation_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count() / 1000.0;

        if (result_value == ReflexxesAPI::RML_FINAL_STATE_REACHED) {
            return Result::Finished;
        } else if (result_value < 0) {
            return Result::Error;
        }
        return Result::Working;
    }

    void at_time(double time, OutputParameter<DOFs>& output) {
        switch (current_input.type) {
        case InputParameter<DOFs>::Type::Position: {
            rml->RMLPositionAtAGivenSampleTime(time, output_parameters.get());

            for (size_t dof = 0; dof < DOFs; dof += 1) {
                output.new_position[dof] = output_parameters->NewPositionVector->VecData[dof];
                output.new_velocity[dof] = output_parameters->NewVelocityVector->VecData[dof];
                output.new_acceleration[dof] = output_parameters->NewAccelerationVector->VecData[dof];
            }
        } break;
        case InputParameter<DOFs>::Type::Velocity: {
            rml->RMLVelocityAtAGivenSampleTime(time, output_vel_parameters.get());

            for (size_t dof = 0; dof < DOFs; dof += 1) {
                output.new_position[dof] = output_vel_parameters->NewPositionVector->VecData[dof];
                output.new_velocity[dof] = output_vel_parameters->NewVelocityVector->VecData[dof];
                output.new_acceleration[dof] = output_vel_parameters->NewAccelerationVector->VecData[dof];
            }
        } break;
        }
    }
};

} // namespace ruckig
