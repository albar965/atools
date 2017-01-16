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
-- Update navigation references for waypoint -------------------
-- *************************************************************

update waypoint set nav_id =
(
  select v.vor_id
  from vor v
  where waypoint.type = 'V' and waypoint.ident = v.ident and waypoint.region = v.region
)
where waypoint.type = 'V';

update waypoint set nav_id =
(
  select n.ndb_id
  from ndb n
  where waypoint.type = 'N' and waypoint.ident = n.ident and waypoint.region = n.region
)
where waypoint.type = 'N';

