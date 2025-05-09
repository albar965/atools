-- *****************************************************************************
-- Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
-- Update number of ILS runway ends in Airport ------------------
-- *************************************************************
drop table if exists tmp_ap_num_ils;

create table tmp_ap_num_ils as
select cast(ap_id as integer) as ap_id, cast(count(1) as integer) as cnt from (
  select r.airport_id as ap_id, primary_end_id as end_id
  from runway r
  join ils i on r.primary_end_id = i.loc_runway_end_id
  where i.gs_range is not null and i.type not in ('G', 'T') -- Only real ILS
  union
  select r2.airport_id as ap_id,secondary_end_id as end_id from runway r2
  join ils i2 on r2.secondary_end_id = i2.loc_runway_end_id
  where i2.gs_range is not null and i2.type not in ('G', 'T') -- Only real ILS
) group by ap_id;

create index if not exists idx_tmp_ap_num_ils on tmp_ap_num_ils(ap_id);

update airport set num_runway_end_ils = (
select r.cnt
from tmp_ap_num_ils r where r.ap_id = airport.airport_id);

update airport set num_runway_end_ils = 0 where num_runway_end_ils is null;

