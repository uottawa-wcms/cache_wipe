<?php

function cache_wipe_admin_form($form, &$form_state) {
  $form['clear_all'] = array(
    '#type' => 'fieldset',
    '#title' => t('Reset External Caches'),
    'clear_all_trigger' => array(
      '#submit' => array('cache_wipe_reset_all'),
      '#value' => t('Reset External Caches'),
      '#description' => t('This will trigger a reset of all external caches'),
      '#type' => 'submit',
      '#limit_validation_errors'=> array(),
    )
  );
  $form['start_batch'] = array(
    '#type' => 'fieldset',
    '#title' => t('Run Batch Now'),
    'start_batch_trigger' => array(
      '#submit' => array('cache_wipe_batch_start'),
      '#value' => t('Run Batch Now'),
      '#description' => t('This will manually start the cache clearing process'),
      '#type' => 'submit',
      '#limit_validation_errors'=> array(),
    )
  );
  $form['global_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Global Settings'),
    'cache_wipe_max_runtime' => array(
      '#type' => 'textfield',
      '#title' => t('Maximum Runtime'),
      '#description' => t('The cron task to wipe external caches is limited to this length of time, in seconds'),
      '#required' => TRUE,
      '#default_value' => variable_get('cache_wipe_max_runtime', 15),
    ),
    'cache_wipe_delay' => array(
      '#type' => 'textfield',
      '#title' => t('Run Delay'),
      '#description' => t('The cron task will not wipe external caches unless this amount of time has passed'),
      '#required' => TRUE,
      '#default_value' => variable_get('cache_wipe_delay', 3600),
    ),
    'cache_wipe_on_clear' => array(
      '#type' => 'checkbox',
      '#title' => t('Clear external caches with internal cache'),
      '#description' => t('If checked, a call to wipe all external caches will be placed when the internal cache is cleared'),
      '#default_value' => variable_get('cache_wipe_on_clear', FALSE),
    ),
  );
  $form['save_settings'] = array(
    '#type' => 'submit',
    '#value' => t('Save Settings'),
  );
  return $form;
}

function cache_wipe_admin_form_validate($form, &$form_state) {
  if (!filter_var($form_state['values']['cache_wipe_max_runtime'], FILTER_VALIDATE_INT)) {
    form_set_error('cache_wipe_max_runtime', t('Maximum Runtime must be an integer'));
  }
  if (!filter_var($form_state['values']['cache_wipe_delay'], FILTER_VALIDATE_INT)) {
    form_set_error('cache_wipe_delay', t('Run Delay must be an integer'));
  }
}

function cache_wipe_admin_form_submit($form, &$form_state) {
  variable_set('cache_wipe_max_runtime', $form_state['values']['cache_wipe_max_runtime']);
  variable_set('cache_wipe_delay', $form_state['values']['cache_wipe_delay']);
  variable_set('cache_wipe_on_clear', !!$form_state['values']['cache_wipe_on_clear']);
  drupal_set_message(t('Settings have been saved'));
}

function cache_wipe_reset_all() {
  _cache_wipe_queue_everything();
  drupal_set_message(t('All external caches will be cleared in the next batch process'));
}

function cache_wipe_batch_start() {
/*
  $batch = array(
    'title' => t('Clearing external caches'),
    'operations' => array(
      array('_cache_wipe_for_batch'),
    ),
    'finished' => 'cache_wipe_batch_finished',
    'file' => 'cache_wipe.inc',
  );
  batch_set($batch);
*/
  module_load_include('inc', 'cache_wipe');
  _cache_wipe_run_queue();
}

function cache_wipe_batch_finished($success, $results, $operations) {
  if ($success) {
    $message = format_plural(count($results), 'One cache item cleared', '@count cache items cleared');
  } else {
    $message = t('Finished with errors');
  }
  drupal_set_message($message);
}