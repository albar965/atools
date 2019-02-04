-- *****************************************************************************
-- Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

-- Create indexes for most searchable fields (paritally case insensitive)

create index if not exists idx_logbook_startdate on logbook(startdate);
create index if not exists idx_logbook_airport_from_icao on logbook(airport_from_icao);
create index if not exists idx_logbook_airport_to_icao on logbook(airport_to_icao);
create index if not exists idx_logbook_description on logbook(description);
create index if not exists idx_logbook_total_time on logbook(total_time);
create index if not exists idx_logbook_night_time on logbook(night_time);
create index if not exists idx_logbook_instrument_time on logbook(instrument_time);
create index if not exists idx_logbook_aircraft_reg on logbook(aircraft_reg);
create index if not exists idx_logbook_aircraft_descr on logbook(aircraft_descr);
create index if not exists idx_logbook_aircraft_type on logbook(aircraft_type);
create index if not exists idx_logbook_aircraft_flags on logbook(aircraft_flags);

create index if not exists idx_logbook_visits_lbd on logbook_visits(logbook_id);
create index if not exists idx_logbook_visits_ap on logbook_visits(airport);
