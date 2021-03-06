/*
 *  Created on: 15 Jul 2014
 *      Author: Yiming Yang
 * 
 * Copyright (c) 2016, University Of Edinburgh 
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 * 
 *  * Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 *  * Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *  * Neither the name of  nor the names of its contributors may be used to 
 *    endorse or promote products derived from this software without specific 
 *    prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include <exotica/Problems/UnconstrainedEndPoseProblem.h>
#include <exotica/Setup.h>

REGISTER_PROBLEM_TYPE("UnconstrainedEndPoseProblem", exotica::UnconstrainedEndPoseProblem)

#define XML_CHECK(x) {xmltmp=handle.FirstChildElement(x).ToElement();if (!xmltmp) throw_named("XML element '"<<x<<"' does not exist!");}

namespace exotica
{
    UnconstrainedEndPoseProblem::UnconstrainedEndPoseProblem()
    {
        Flags = KIN_FK | KIN_J;
    }

    UnconstrainedEndPoseProblem::~UnconstrainedEndPoseProblem()
    {
    }

    void UnconstrainedEndPoseProblem::Instantiate(UnconstrainedEndPoseProblemInitializer& init)
    {
        Tasks = MapToVec(TaskMaps);
        NumTasks = Tasks.size();
        Mapping.resize(NumTasks, 2);
        int id=0;
        for(int i=0;i<NumTasks;i++)
        {
            Mapping(i,0) = id;
            Mapping(i,1) = Tasks[i]->taskSpaceDim();
            id += Mapping(i,1);            
        }
        PhiN = Mapping.col(1).sum();

        N = scene_->getNumJoints();

        if(init.W.rows()!=N) throw_named("W dimension mismatch! Expected "<<N<<", got "<<init.W.rows());

        Rho = Eigen::VectorXd::Ones(NumTasks);
        y = Eigen::VectorXd::Zero(PhiN);
        W = Eigen::MatrixXd::Identity(N, N);
        W.diagonal() = init.W;
        Phi = Eigen::VectorXd::Zero(PhiN);
        J = Eigen::MatrixXd(PhiN, N);

        if(init.Rho.rows()>0 && init.Rho.rows()!=NumTasks) throw_named("Invalide size of Rho (" << init.Rho.rows() << ") expected: "<< NumTasks);
        if(init.Goal.rows()>0 && init.Goal.rows()!=PhiN) throw_named("Invalide size of Rho (" << init.Goal.rows() << ") expected: "<< PhiN);

        if(init.Rho.rows()==NumTasks) Rho = init.Rho;
        if(init.Goal.rows()==PhiN) y = init.Goal;
    }

    void UnconstrainedEndPoseProblem::Update(Eigen::VectorXdRefConst x)
    {
        scene_->Update(x);
        for(int i=0;i<NumTasks;i++)
        {
            Tasks[i]->update(x, Phi.segment(Mapping(i, 0), Mapping(i, 1)), J.middleRows(Mapping(i, 0), Mapping(i, 1)));
        }
    }

    void UnconstrainedEndPoseProblem::setGoal(const std::string & task_name, Eigen::VectorXdRefConst goal)
    {
        TaskMap_ptr task = TaskMaps.at(task_name);
        if(goal.rows()!=task->Length) throw_named("Invalid goal dimension "<<goal.rows()<<" expected "<<task->Length);
        y.segment(task->Start, task->Length) = goal;
    }

    void UnconstrainedEndPoseProblem::setRho(const std::string & task_name, const double rho)
    {
        TaskMap_ptr task = TaskMaps.at(task_name);
        Rho(task->Id) = rho;
    }

    Eigen::VectorXd UnconstrainedEndPoseProblem::getGoal(const std::string & task_name)
    {
        TaskMap_ptr task = TaskMaps.at(task_name);
        return y.segment(task->Start, task->Length);
    }

    double UnconstrainedEndPoseProblem::getRho(const std::string & task_name)
    {
        TaskMap_ptr task = TaskMaps.at(task_name);
        return y(task->Id);
    }
}

