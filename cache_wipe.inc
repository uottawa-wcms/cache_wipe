<?php

function _cache_wipe_run_queue() {
  $end_time = time() + variable_get('cache_wipe_max_runtime', 15);
  $continue = TRUE;
  $queue = UniqueQueue::get('cache_wipe_objects');
  while ($continue) {
    $queue_item = $queue->claimItem();
    if (!empty($queue_item)) {
print 'processing';
      _cache_wipe_process_data_node($queue_item->data);
      $queue->deleteItem($queue_item);
    } else {
      $continue = FALSE;
    }
    $continue = $continue && !!$queue->itemsLeft();
    $continue = $continue && $end_time > time();
  }
}

function _cache_wipe_process_data_node($data) {

  $everything = drupal_static(__FUNCTION__, FALSE);
  if ($everything) {
    return;
  }
  if (!empty($data)) {
    switch ($data['mode']) {
      case CACHE_WIPE_ALL:
        _cache_wipe_everything();
        $everything = TRUE;
        break;
      case CACHE_WIPE_SINGLE:
      default:
        module_load_include('inc', 'path_usage');
        _cache_wipe_process_item($data);
        break;
    }
  }
}

function _cache_wipe_for_batch(&$context) {
  $queue = UniqueQueue::get('cache_wipe_objects');
  if (empty($context['sandbox'])) {
    $context['sandbox'] = array(
      'progress' => 0,
      'max' => $queue->itemsLeft(),
    );
  }
  $continue = TRUE;
  $count = 0;
  while ($continue) {
    $item = $queue->claimItem();
    if (!empty($item)) {
      _cache_wipe_process_data_node($item->data);
      $queue->deleteItem($item);
    } else {
      $continue = FALSE;
    }
    $count++;
    $continue = $continue && ($count < 10);
    $continue = $continue && $queue->itemsLeft();
    $context['sandbox']['progress']++;
    if ($item->data['mode'] == CACHE_WIPE_SINGLE) {
      $context['results'][] = check_plain($item->data['type'] . ':' . $item->data['id']);
      $context['message'] = check_plain($item->data['type'] . ':' . $item->data['id']);
    } else {
      $context['results'][] = t('Everything');
      $context['message'] = t('Everything');
    }
  }
  if ($context['sandbox']['progress'] != $context['sandbox']['maximum']) {
    $context['sandbox']['finished'] = $context['sandbox']['progress'] /
        $context['sandbox']['max'];
  }
}

function _cache_wipe_everything() {
  // move to system hook
  db_truncate('cache_page')->execute();
  $modules = module_implements('cache_wipe_all');
  foreach ($modules as $module) {
    $hook = $module . '_cache_wipe_all';
    if (function_exists($hook)) {
      $hook();
    }
  }
  UniqueQueue::get('cache_wipe_objects')->deleteQueue();
}

function _cache_wipe_process_item($item) {
  global $base_url;
  $paths = path_usage_find_paths(
      $item['object'],
      $item['id'],
      $item['type'],
      $item['subtype']
  );
  $full_paths = array();
  foreach ($paths as $path) {
    if (substr($path, 0, 1) == '/') {
      $full_paths[] = $base_url . $path;
    } elseif (strpos($path, '://') === FALSE) {
      $full_paths[] = $base_url . '/' . $path;
    } else {
      $full_paths[] = $path;
      $full_paths[] = str_replace('https://', 'http://', $path);
    }
  }
drupal_set_message('clearing paths: ' . implode('-----', $full_paths));
  db_delete('cache_page')
      ->condition('cid', $full_paths, 'IN')
      ->execute();
  drupal_set_message('wiping page cache');
  $modules = module_implements('cache_wipe');
  foreach ($modules as $module) {
    $hook = $module . '_cache_wipe';
    if (function_exists($hook)) {
      $hook($paths);
    }
  }
}