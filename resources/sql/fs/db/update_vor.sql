-- *****************************************************************************
-- Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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
-- Merge VOR and TACAN to VORTAC -------------------
-- *************************************************************

-- Copy channel from TACAN
update vor set channel = (
select v2.channel from vor v1
left outer join vor v2 on v1.ident = v2.ident and v1.region = v2.region and v1.lonx = v2.lonx and v1.laty = v2.laty
where
v1.type != 'TC' and v2.type = 'TC' and v1.vor_id = vor.vor_id)
where vor.channel is null and vor.type != 'TC';

-- Set new type to VORTAC
update vor set type = 'VTH' where vor_id in (
select v1.vor_id from vor v1
left outer join vor v2 on v1.ident = v2.ident and v1.region = v2.region and v1.lonx = v2.lonx and v1.laty = v2.laty
where
v1.type = 'H' and v2.type = 'TC' and v1.vor_id = vor.vor_id);

-- Set new type to VORTAC
update vor set type = 'VTL' where vor_id in (
select v1.vor_id from vor v1
left outer join vor v2 on v1.ident = v2.ident and v1.region = v2.region and v1.lonx = v2.lonx and v1.laty = v2.laty
where
v1.type = 'L' and v2.type = 'TC' and v1.vor_id = vor.vor_id);

-- Set new type to VORTAC
update vor set type = 'VTT' where vor_id in (
select v1.vor_id from vor v1
left outer join vor v2 on v1.ident = v2.ident and v1.region = v2.region and v1.lonx = v2.lonx and v1.laty = v2.laty
where
v1.type = 'T' and v2.type = 'TC' and v1.vor_id = vor.vor_id);


-- delete the now useless TACANS
delete from vor where vor_id in (
select v2.vor_id from vor v1
left outer join vor v2 on v1.ident = v2.ident and v1.region = v2.region and v1.lonx = v2.lonx and v1.laty = v2.laty
where v1.type != 'TC' and v2.type = 'TC');
