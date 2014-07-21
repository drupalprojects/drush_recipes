<?php

/**
 * @file
 * Drush Recipe format.
 *
 * This format allows you to call a series of drush calls to fire in
 * succession. This allows them to be packaged along with modules and
 * themes and is a complementary format to .make files. Where .make is
 * for describing how to make a site. drush recipe files are for producing a
 * structured, reusable, desired result.
 *
 * Much of what you can do with drecipes can be done via features but
 * this obviously is far less packaging cruft. It also allows you to be
 * intentionally destructive with your calls instead of worrying about
 * being in "override hell" with features and features_override.
 */

define('DRUSH_RECIPE_EXTENSION', 'drecipe');
define('DRUSH_RECIPES_API_VERSION', '1.0');
define('DRUSH_RECIPES_CORE_COMPATIBILITY', '7.x');
define('DRUSH_RECIPES_MAX_RECURSION', 4);

/**
 * Implements hook_drush_command().
 */
function drush_recipes_drush_command() {
  $items = array();
  $items['drush-list-recipes'] = array(
    'description' => dt('List the available recipes and where they are loaded from'),
    'arguments' => array(
      'list' => dt('Recipies to list, comma separated'),
    ),
    'options' => array(
      'more-detail' => dt('Show all info available'),
    ),
    'aliases' => array('dlr'),
    'examples' => array(
      'drush drush-list-recipes' =>
        'A nicely printed listing of available recipes',
      'drush dlr dr_security,dr_admin_update_status' =>
        'Only list these two recipes',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
  );
  $items['drush-write-recipe'] = array(
    'description' => dt('Interactive prompt to create a new recipe'),
    'aliases' => array('dwr'),
    'examples' => array(
      'drush dwr' =>
        'An interactive prompt will follow',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['drush-cook-recipes'] = array(
    'description' => dt('"cook" up a drush recipe'),
    'arguments' => array(
      'recipes' => dt("List of recipes to execute, in order and comma separated"),
    ),
    'aliases' => array('cook'),
    'examples' => array(
      'drush drush-cook-recipes dr_security' =>
        'Run the dr_security recipe',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
  );
  return $items;
}

/**
 * Implements hook_drush_help().
 */
function drush_recipes_drush_help($section) {
  switch ($section) {
    case 'drush:drush-list-recipes':
      return dt('List the available recipes');
    case 'drush:drush-cook-recipes':
      return dt('Perform a series of drush recipes');
    case 'drush:drush-write-recipe':
      return dt('Write a new recipe from commandline');
  }
}

/**
 * Drush command callback for drush-write-recipe.
 */
function drush_drush_recipes_drush_write_recipe() {
  $machine_name = drush_prompt(dt('Enter a machine name'));
  //get the first character of machine name and check if it is a number
  $first_char_machine_name = substr($machine_name, 0, 1);
  if (!ctype_alpha($first_char_machine_name)){
    //echo that input cannot start with a number.
    drush_log('Syntax Error: Please enter a machine name that does not start with a number.', 'error');
    // we return to effectively start a recursive loop but cancel out this one.
    return drush_drush_recipes_drush_write_recipe();
  }
  else {
    $original_machine = $machine_name;
    $machine_name = strtolower($machine_name);
    // check for spaces and replace with underscores.
    $machine_name = preg_replace("/[^a-z0-9_]/" , '_', $machine_name);
    // verify that user wants the converted string; if not start over.
    if (($original_machine != $machine_name) && !drush_confirm(dt('Is @machine the machine name you want?', array('@machine' => $machine_name)))) {
      return drush_drush_recipes_drush_write_recipe();
    }
  }
  $name = drush_prompt(dt('Enter recipe name'));
  // description and version are optional
  $description = drush_prompt(dt('Enter a description'), NULL, FALSE);
  $version = drush_prompt(dt('Enter version'), '1.0');
  $options = array(
    1 => 'add-on',
    2 => 'routine',
  );
  // prompt for type choice
  while (!$type = drush_choice($options, dt('What type of recipe is this?')))
  $type = $options[$type];
  // author is optional
  $author = drush_prompt(dt('Recipe author'), NULL, FALSE);
  $call = '';
  $items = array();
  while ($call != 'x') {
    $call = drush_prompt(dt('Write the full drush command you want to add (type x when done)'));
    // if we didn't get told to stop
    if ($call != 'x') {
      // convert drush call into array using spaces
      $item = explode(' ', $call);
      // this means its a reference to an another drecipe
      if (count($item) == 1 && strpos($item[0], '.' . DRUSH_RECIPE_EXTENSION)) {
        $item = (string)$item[0];
      }
      elseif ($item[0] == 'drush') {
        array_shift($item);
      }
      else {
        // ignore input
        drush_log(dt('Invalid command'), 'warning');
        continue;
      }
      // add to array
      $items[] = $item;
    }
  }
  $recipe = array(
    'name' => $name,
    'drush_recipes_api' => DRUSH_RECIPES_API_VERSION,
    'weight' => 0,
    'recipe' => $items,
    'metadata' => array(
      'type' => $type,
      'description' => $description,
      'version' => $version,
      'author' => $author,
    )
  );

  $formats = array(
    'json' => 'json',
    'xml' => 'xml'
  );
  // test for yaml since we support that format too
  if (function_exists('yaml_emit')) {
    // allow for writing to file or screen
    $formats['yaml'] = 'yaml';
  }
  while (!$format = drush_choice($formats, dt('Which recipe encoding?')))
  // allow for writing to file or screen
  $options = array(
    1 => 'print',
    2 => 'write to file',
  );
  // start writing to file if they selected that
  if (drush_choice($options, dt('What type of recipe is this?')) == 2) {
    $file = $machine_name . '.' . DRUSH_RECIPE_EXTENSION;
    $location = drush_prompt(dt('Location to write recipe'), drush_get_context('DRUSH_PER_USER_CONFIGURATION') . '/drecipes');
    // test directory exists / create it if we can
    if (is_dir("$location") || mkdir("$location")) {
      // try to write recipe to the file
      $output = _drush_recipes_pretty_json(_drush_recipes_encode($recipe, $format));
      if (!file_put_contents("$location/$file", $output)) {
        drush_log(dt("Unable to write file @file", array('@file' => $file)), 'error');
        drush_print(_drush_recipes_encode($recipe, $format));
      }
      else {
        // success! tell them how to use this
        drush_log(dt("New recipe written to @file. call `drush cook @machine` to cook up this recipe.", array('@file' => "$location/$file", '@machine' => $machine_name)), 'ok');
      }
    }
    else {
      drush_log(dt("Unable to write file @file", array('@file' => $file)), 'error');
      drush_print(_drush_recipes_encode($recipe, $format));
    }
  }
  else {
    // just write it to the screen though this is mostly just for debugging
    drush_print(_drush_recipes_encode($recipe, $format));
  }
  return TRUE;
}

/**
 * Drush command callback for drush-list-recipes.
 */
function drush_drush_recipes_drush_list_recipes($list = '') {
  $recipes = _drush_recipes_system_rebuild_recipe_data();
  $list = (!empty($list) ? explode(',', $list) : '');
  $rows = array();
  // get option flag for more detail
  $more = drush_get_option('more-detail');
  // make sure we find any
  if (empty($recipes)) {
    drush_log(dt('You have no recipes! Run `drush dwr` to create a new one!'), 'error');
    return FALSE;
  }
  $header = $rows = array();
  // format recipe data as a table array
  foreach ($recipes as $machine_name => $recipe) {
    $row = $recipe->drecipe;
    // clean up data for output
    if ($more) {
      $row['dependencies'] = implode(' ', $row['dependencies']);
      $row['conflicts'] = implode(' ', $row['conflicts']);
      $row['recipe'] = implode("\n", _drush_recipes_recipe_to_drush($row['recipe']), 0, FALSE, FALSE);
      $row['metadata'] = implode(' ', $row['metadata']);
    }
    else {
      unset($row['dependencies']);
      unset($row['conflicts']);
      unset($row['recipe']);
      unset($row['metadata']);
      unset($row['core']);
      unset($row['drush_recipes_api']);
      unset($row['weight']);
    }
    $row['machine_name'] = $machine_name;
    $row['uri'] = $recipe->filename;
    // allow for limiting what they care about from commandline
    if (!empty($list) && !in_array($machine_name, $list)) {
      continue;
    }
    $rows[] = $row;
  }
  if (empty($rows)) {
    drush_log(dt('No results matched the list you wanted to see'), 'error');
    return FALSE;
  }
  array_unshift($rows, array_keys($row));
  // pretty print the columns
  if ($more) {
    $widths = array(
      'uri' => '30',
      'machine_name' => '8',
      'name' => '10',
      'conflicts' => '10',
      'core' => '3',
      'drush_recipes_api' => '3',
      'metadata' => '30',
      'recipe' => '40',
      'dependencies' => '10',
      'weight' => '2',
    );
  }
  else {
    $widths = array();
  }
  drush_print_table($rows, TRUE, $widths);
}

/**
 * Drush command callback for drush-cook-recipes.
 */
function drush_drush_recipes_drush_cook_recipes($list = '') {
  $commands = array();
  $list = explode(',', $list);
  if (empty($list)) {
    drush_log('You must specify recipes to cook! try running drush dlr for a listing of commands', 'error');
    return FALSE;
  }
  $recipes = _drush_recipes_system_rebuild_recipe_data();
  $cook = array_intersect_key($recipes, array_flip($list));
  // @todo utilize the recipe weight to ensure correct weighted firing order
  // This should only be used when all defined at once, cookbook style recipes
  // need to treat them sequentially.
  foreach ($cook as $recipe) {
    $commands += _drush_recipes_recipe_to_drush($recipe->drecipe['recipe']);
  }
  // confirm cooking as this can be a ton of drush commands
  if (drush_confirm('Are you sure you want to execute these recipes with the above call structure? ' . implode(' ', array_keys($cook)))) {
    foreach ($commands as $command) {
      $command .= ' --y';
      //@todo cook the recipies via drush_invoke not sure which
      drush_log('Adding ingredient: ' . $command, 'ok');
      // this is the simple method for running
      drush_shell_exec($command);
      $shell_output = drush_shell_exec_output();
      foreach ($shell_output as $shell) {
        drush_print($shell);
      }
      drush_log($command . ' complete!', 'ok');
    }
  }
  else {
    return drush_user_abort();
  }
}

/**
 * Helper to rebuild drecipe data
 *
 * This is similar to how core handles modules / themes but forked to provide
 * support for modules, themes, and "drecipes" directory structures.
 */
function _drush_recipes_system_rebuild_recipe_data() {
  // // we allow for this command to run regardless of site scope
  $bootstrap = drush_get_context('DRUSH_BOOTSTRAP_PHASE');
  if ($bootstrap != DRUSH_BOOTSTRAP_DRUSH) {
    // test for static cache of output
    $recipes = &drupal_static(__FUNCTION__);
    if (isset($recipes)) {
      return $recipes;
    }
    // find recipes in drush specific locations
    $locations = array(
      'drush-system',
      'drecipes',
      'themes',
      'modules',
    );
    $function_pattern = DRUPAL_PHP_FUNCTION_PATTERN;
  }
  else {
    // we can only check in the drush sub-system for recipes
    $locations = array(
      'drush-system',
    );
    $function_pattern = '[a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*';
  }
  $recipes = array();
  // file preg_match pattern
  $pattern = '/^' . $function_pattern . '\.' . DRUSH_RECIPE_EXTENSION . '/';
  // loop through each directory and find recipes
  foreach ($locations as $directory) {
    // find recipes in the drecipes directory
    $recipes += _drush_recipes_drupal_system_listing($pattern, $directory, 'name', 0);
  }
  // Set defaults for recipes.
  $defaults = array(
    'drush_recipes_api' => DRUSH_RECIPES_API_VERSION,
    'weight' => 0,
    'core' => DRUSH_RECIPES_CORE_COMPATIBILITY,
    'dependencies' => array(),
    'conflicts' => array(),
    'recipe' => array(),
    'metadata' => array(),
  );

  // Read recipe files for each module/theme/drecipes.
  foreach ($recipes as $key => $recipe) {
    // Look for the info file.
    $recipe->drecipe = _drush_recipes_load_recipe(dirname($recipe->filename) . '/' . $recipe->basename);
    // Skip $recipes that don't provide info or are malformed.
    if (empty($recipe->drecipe) || empty($recipe->drecipe['recipe'])) {
      unset($recipes[$key]);
      continue;
    }
    // Merge in defaults and save.
    $recipes[$key]->drecipe = $recipe->drecipe + $defaults;
    krsort($recipes[$key]->drecipe);
  }
  return $recipes;
}

/**
 * Fork of drupal_system_listing to parse files
 * @param  [type]  $mask      [preg_match for file pattern matching]
 * @param  [type]  $directory [directory to search in]
 * @param  string  $key       [file key]
 * @param  integer $min_depth [depth into the directory to traverse]
 * @return [array]             [an array of objects relating to recipe files]
 */
function _drush_recipes_drupal_system_listing($mask, $directory, $key = 'name', $min_depth = 0) {
  $searchdir = array();
  $files = array();
  // drush based paths need to be discovered in a different manner
  if ($directory == 'drush-system') {
    $searchdir[] = drush_get_context('DRUSH_PER_USER_CONFIGURATION');
    $searchdir[] = drush_get_context('DRUSH_SITE_WIDE_CONFIGURATION');
    $searchdir[] = dirname(__FILE__) . '/..';
  }
  else {
    $searchdir = array($directory);
    $profiles = array();
    $profile = drupal_get_profile();
    // In case both profile directories contain the same extension, the actual
    // profile always has precedence.
    $profiles[] = $profile;
    foreach ($profiles as $profile) {
      if (file_exists("profiles/$profile/$directory")) {
        $searchdir[] = "profiles/$profile/$directory";
      }
    }

    // Always search sites/all/* as well as the global directories.
    $searchdir[] = 'sites/all/' . $directory;
    $config = conf_path();
    if (file_exists("$config/$directory")) {
      $searchdir[] = "$config/$directory";
    }
  }
  foreach ($searchdir as $dir) {
    $files_to_add = drush_scan_directory($dir, $mask, array('.', '..', 'CVS'), 0, TRUE, $key, $min_depth);
    // Duplicate files found in later search directories take precedence over
    // earlier ones, so we want them to overwrite keys in our resulting
    // $files array.
    // The exception to this is if the later file is from a module or theme not
    // compatible with Drupal core. This may occur during upgrades of Drupal
    // core when new modules exist in core while older contrib modules with the
    // same name exist in a directory such as sites/all/modules/.
    foreach (array_intersect_key($files_to_add, $files) as $file_key => $file) {
      // If it has no drecipe file, then we just behave liberally and accept the
      // new resource on the list for merging.
      if (file_exists($recipe_file = dirname($file->filename) . '/' . $file->basename)) {
        // Get the .DRUSH_RECIPE_EXTENSION file for the module or theme this file belongs to.
        $recipe = _drush_recipes_load_recipe($recipe_file);
        // If the module or theme is incompatible with Drupal core, remove it
        // from the array for the current search directory, so it is not
        // overwritten when merged with the $files array.
        if (isset($recipe['core']) && $recipe['core'] != DRUSH_RECIPES_CORE_COMPATIBILITY) {
          unset($files_to_add[$file_key]);
        }
      }
    }
    $files = array_merge($files, $files_to_add);
  }

  return $files;
}

/**
 * Helper function to load a file.
 *
 * @return array decoded array from one of the formats listed.
 */
function _drush_recipes_load_recipe($path) {
  $contents = file_get_contents($path);
  // ensure utf8 to avoid issues
  $contents = utf8_encode($contents);
  // to avoid issues w/ format extension names we trickle down based on support
  $recipe = @drush_json_decode($contents);
  // if empty, lets try a yaml parser
  if (empty($recipe)) {
    // @todo add support for XML cause... why not
    // test for yaml capabilities
    if (function_exists('yaml_parse')) {
      $recipe = @yaml_parse($contents);
    }
  }
  // lets try xml as a last ditch
  if (empty($recipe)) {
    $recipe = @simplexml_load_string($contents);
  }
  else {
    // something went wrong, we couldn't read the file
    drush_log(dt('There is a problem with the recipe located at @file', array('@file' => $path)), 'error');
    return FALSE;
  }
  return $recipe;
}

/**
 * Helper function to export to a certain format.
 *
 * @return array encoded contents for export.
 */
function _drush_recipes_encode($contents, $format = DRUSH_RECIPES_FORMAT_DEFAULT) {
  switch ($format) {
    case 'json':
      return drush_json_encode($contents);
    break;
    case 'yaml':
      if (function_exists('yaml_emit')) {
        return yaml_emit($contents);
      }
    break;
    case 'xml':
      $xml = new SimpleXMLElement('<drush_recipe/>');
      array_walk_recursive($contents, array($xml, 'addChild'));
      return $xml->asXML();
    break;
  }
  return FALSE;
}

/**
 * convert a recipe array to drush calls
 * @param  array $recipe  a structured array of drush calls in recipe format
 * @param  bool  $recurse if the functionc all should be recursive
 * @param  bool  $list    if calls should be printed as they are found
 * @return array $drush   array of drush calls in order they should execute
 */
function _drush_recipes_recipe_to_drush($recipe, $indent = 0, $recurse = TRUE, $list = TRUE) {
  // catch to make sure we don't fall into a deep infinite loop of recursive calls
  if ($indent > DRUSH_RECIPES_MAX_RECURSION) {
    // tell people in the event they triggered this and are confused
    drush_log(dt('There is most likely a problem with your logic for this recipe series. It was about to make a call 5 levels down in its referenced call logic, and prevented this from happening to avoid an infinite loop. If you know what you are doing, increase DRUSH_RECIPES_MAX_RECURSION. If you think the current value is too low, please file an issue in the issue queue.'), 'error');
    return array();
  }
  $drush = array();
  foreach ($recipe as $call) {
    // drush commands are listed as array'ed elements
    if (is_array($call)) {
      $drush[] = 'drush ' . implode(' ', $call);
      // see if we should print nicely formatted debug stuff
      if ($list) {
        drush_print(_drush_recipes_indent($indent) . 'drush ' . implode(' ', $call));
      }
    }
    else {
      // we have a recipe nested, lets look up how to build it and recurse
      // but only if told to do so, may just want to reference the command
      if ($recurse) {
        // pull data for full object, fortunately it's static cached
        $recipes = _drush_recipes_system_rebuild_recipe_data();
        // loop through items to cross reference with filename
        foreach ($recipes as $machine_name => $item) {
          // if call matches filename we recurse through that filename's recipe
          if ($machine_name == str_replace('.' . DRUSH_RECIPE_EXTENSION, '', $call)) {
            if ($list) {
              drush_print(_drush_recipes_indent($indent) . '<' . $machine_name . '>');
            }
            $drush += _drush_recipes_recipe_to_drush($item->drecipe['recipe'], $indent+1);
            if ($list) {
              drush_print(_drush_recipes_indent($indent) . '</' . $machine_name . ">\n");
            }
            continue;
          }
        }
      }
      else {
        $drush[] = 'drush der '  . str_replace('.' . DRUSH_RECIPE_EXTENSION, '', $call);
      }
    }
  }
  return $drush;
}

/**
 * Helper for indenting
 */
function _drush_recipes_indent($count) {
  $output = '';
  $counter = 0;
  while ($counter < $count) {
    $output .= '  ';
    $counter++;
  }
  return $output;
}
/**
 * Print json nicely regardless of PHP version
 */
function _drush_recipes_pretty_json($json) {
  $result = '';
  $level = 0;
  $in_quotes = false;
  $in_escape = false;
  $ends_line_level = NULL;
  $json_length = strlen($json);
  for ($i = 0; $i < $json_length; $i++) {
    $char = $json[$i];
    $new_line_level = NULL;
    $post = "";
    if ($ends_line_level !== NULL) {
      $new_line_level = $ends_line_level;
      $ends_line_level = NULL;
    }
    if ($in_escape) {
      $in_escape = false;
    }
    elseif ($char === '"') {
      $in_quotes = !$in_quotes;
    }
    elseif (!$in_quotes) {
      switch ($char) {
        case '}':
        case ']':
          $level--;
          $ends_line_level = NULL;
          $new_line_level = $level;
        break;

        case '{':
        case '[':
          $level++;
        case ',':
          $ends_line_level = $level;
        break;

        case ':':
          $post = " ";
        break;

        case " ":
        case "\t":
        case "\n":
        case "\r":
          $char = "";
          $ends_line_level = $new_line_level;
          $new_line_level = NULL;
        break;
      }
    }
    elseif ($char === '\\') {
      $in_escape = true;
    }

    if ($new_line_level !== NULL) {
        $result .= "\n".str_repeat("\t", $new_line_level);
    }
    $result .= $char . $post;
  }

  return $result;
}
