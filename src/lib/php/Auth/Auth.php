<?php
/*
Copyright (C) 2014-2015, Siemens AG

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

namespace Fossology\Lib\Auth;

class Auth {
  const USER_NAME = 'User';
  const USER_ID = 'UserId';
  const GROUP_ID = 'GroupId';
  const USER_LEVEL = 'UserLevel';

  /**
   * Permissions
   * See http://www.fossology.org/projects/fossology/wiki/PermsPt2
   */
  const PERM_NONE = 0;
  const PERM_READ = 1;
  const PERM_WRITE= 3;
  const PERM_ADMIN=10;

  public static function getUserId()
  {
    return $GLOBALS['SysConf']['auth'][self::USER_ID];
  }
  
  public static function getGroupId()
  {
    return $GLOBALS['SysConf']['auth'][self::GROUP_ID];
  }
  
  public static function isAdmin()
  {
    return $_SESSION[self::USER_LEVEL]==self::PERM_ADMIN;
  }
}
