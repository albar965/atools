-- *****************************************************************************
-- Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

-- *************************************************************
-- Update ILS runway ids
-- *************************************************************

update ils set loc_runway_end_id = (
  select runway_end_id
  from airport a
  join runway r on r.airport_id = a.airport_id
  join runway_end e on r.primary_end_id = e.runway_end_id
  where e.name = ils.loc_runway_name and a.ident = ils.loc_airport_ident
);

update ils set loc_runway_end_id = (
  select runway_end_id
  from airport a
  join runway r on r.airport_id = a.airport_id
  join runway_end e on r.secondary_end_id = e.runway_end_id
  where e.name = ils.loc_runway_name and a.ident = ils.loc_airport_ident
) where ils.loc_runway_end_id is null;


-- *************************************************************
-- Update number of ILS runway ends in aiport ------------------
-- *************************************************************
drop table if exists temp_ap_num_ils;

create table temp_ap_num_ils as
select cast(ap_id as integer) as ap_id, cast(count(1) as integer) as cnt from (
  select r.airport_id as ap_id, primary_end_id as end_id
  from runway r
  join ils i on r.primary_end_id = i.loc_runway_end_id
  where i.gs_range is not null
  union
  select r2.airport_id as ap_id,secondary_end_id as end_id from runway r2
  join ils i2 on r2.secondary_end_id = i2.loc_runway_end_id
  where i2.gs_range is not null
) group by ap_id;

create index if not exists idx_temp_ap_num_ils on temp_ap_num_ils(ap_id);

update airport set num_runway_end_ils = (
select r.cnt
from temp_ap_num_ils r where r.ap_id = airport.airport_id);

drop table if exists temp_ap_num_ils;

update airport set num_runway_end_ils = 0 where num_runway_end_ils is null;

