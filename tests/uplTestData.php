<?php
/***********************************************************
 Copyright (C) 2008 Hewlett-Packard Development Company, L.P.

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
 ***********************************************************/

/**
 * uplTestData
 * \brief Upload Test data to the repo
 *
 * Upload using upload from file, url.  Sets Mime-type, nomos and package
 * agents.
  *
 * @param URL obtained from the test enviroment globals
 *
 * @version "$Id$"
 *
 * Created on Aug 15, 2008
 */

require_once ('fossologyTestCase.php');
require_once ('TestEnvironment.php');

global $URL;
global $PROXY;

class uploadTestDataTest extends fossologyTestCase
{
  public $mybrowser;
  public $webProxy;

    function setUp()
  {
    global $URL;
    $this->Login();
  }

  /**
   * create the Testing folder used by other tests
   */
  function testCreateTestingFolder()
  {
    global $URL;
    print "Creating Testing folder\n";
    $page = $this->mybrowser->get($URL);
    $this->createFolder(null, 'Testing', null);

    print "Creating Copyright folder\n";
    $this->createFolder(null, 'Copyright', null);
  }

  function testuploadTestDataTest() {

    global $URL;
    global $PROXY;

    print "starting testUploadTestData\n";
    $rootFolder = 1;
    $upload = NULL;
    $uploadList = array('TestData/archives/fossI16L335U29.tar.bz2',
                        'TestData/archives/foss23D1F1L.tar.bz2',
                        'TestData/licenses/gplv2.1',
                        'TestData/licenses/Affero-v1.0',
                        'nomos/testdata/zend-license',
                        'nomos/testdata/RedHat.tar.gz');

    $urlList = array('http://downloads.sourceforge.net/simpletest/simpletest_1.0.1.tar.gz',
                     'http://www.gnu.org/licenses/gpl-3.0.txt',
                     'http://www.gnu.org/licenses/agpl-3.0.txt',
                     'http://filch.ostt/~markd/fossDirsOnly.tar.bz2');

    /* upload the archives using the upload from file menu
     *
     * 1 = bucket agent
     * 2 = copyright agent
     * 3 = mime agent
     * 4 = metadata agent
     * 5 = nomos agent
     * 6 = package agent
     */

    print "Starting file uploads\n";
    foreach($uploadList as $upload) {
      $description = "File $upload uploaded by Upload Test Data Test";
      $this->uploadFile('Testing', $upload, $description, null, '1,2,3,4');
    }

    /* Upload the urls using upload from url.  Check if the user specificed a
     * web proxy for the environment.  If so, set the attribute. */

    if(!(empty($PROXY)))
    {
      $this->webProxy = $PROXY;
    }
    print "Starting Url uploads\n";
    foreach($urlList as $url)
    {
      $this->uploadUrl($rootFolder, $url, null, null, '1,2,3,4,5');
    }
  }
}
?>