-- *****************************************************************************
-- Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
-- ****************************************************************************/


drop table if exists logbook;

drop index if exists idx_logbook_aircraft_name logbook;
drop index if exists idx_logbook_aircraft_type logbook;
drop index if exists idx_logbook_aircraft_registration logbook;
drop index if exists idx_logbook_flightplan_number logbook;
drop index if exists idx_logbook_flightplan_cruise_altitude logbook;
drop index if exists idx_logbook_distance logbook;
drop index if exists idx_logbook_distance_flown logbook;
drop index if exists idx_logbook_departure_ident logbook;
drop index if exists idx_logbook_departure_name logbook;
drop index if exists idx_logbook_departure_lon Xlogbook;
drop index if exists idx_logbook_departure_laty logbook;
drop index if exists idx_logbook_departure_time logbook;
drop index if exists idx_logbook_departure_time_sim logbook;
drop index if exists idx_logbook_destination_ident logbook;
drop index if exists idx_logbook_destination_name logbook;
drop index if exists idx_logbook_destination_lon Xlogbook;
drop index if exists idx_logbook_destination_laty logbook;
drop index if exists idx_logbook_destination_time logbook;
drop index if exists idx_logbook_destination_time_sim logbook;
drop index if exists idx_logbook_simulator on logbook(simulator);
