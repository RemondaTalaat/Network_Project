//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package network_project;

//
// TODO auto-generated module
//
simple Hub
{
    parameters:
        double n                = default(2);                  // number of nodes
        double session_time     = default(15);                 // session time (default = 15 second)
        double stats_time       = default(180);                // statistics print time (default = 180 second (3 mins))
		double prob_duplication = default(5);                
		double prob_modification= default(5);
		double lambda_delay     = default(4.0);                // generate the amount of delay added to the message 
    gates:
        input ins[];
        output outs[];
}
