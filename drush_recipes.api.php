<?php
  /**
   * Drush Recipes API
   *
   * This documents ways you can modify the behavior of drush recipes as well
   * as how to create your own recipes.
   */

/**
 * Drush Recipe 1.0-draft
 *
 * name - Human reable name of this recipe
 * drush_recipes_api - api version, 1.0 defaults
 * weight - weight relative to other recipes if called in the same block-chain
 *   this defaults to 0
 * dependencies - modules required to use this, currently module en dependencies
 *   dictate this though in the future they will be enabled ahead of time.
 * recipe - the structure of commands to execute, this can also be another
 *   recipe filename which will append all the commands in that file ahead of
 *   what is about to execute.
 * metadata - a series of properties that can be used for front-end integration.
 *   this is entirely optional but helps developers understand what you wrote.
 */
$js = <<<JS
{
  "name": "Security defaults",
  "drush_recipes_api": "1.0",
  "weight": 0,
  "dependencies": [
    "seckit",
    "paranoia"
  ],
  "recipe": [
    "dr_admin_update_status.drecipe",
    [
      "vset",
      "user_register",
      "1"
    ],
    [
      "dis",
      "php"
    ],
    [
      "pm-uninstall",
      "php"
    ],
    [
      "en",
      "seckit"
    ],
    [
      "en",
      "paranoia"
    ]
  ],
  "metadata": {
    "descrption": "Disable projects that cause issues, increase h-our defenses cap'in!",
    "version": "1.0",
    "type": "add-on",
    "author": "btopro",
    "logo": "files\/image.png"
  }
}
JS;
