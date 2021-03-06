<?php
// $Id$

/**
 * @file
 * Template to render a Ting collection of books.
 */
?>
  <li>

    <div class="picture">
      <?php $image_url = ting_covers_collection_url($collection->objects[0], '80_x'); ?>
      <?php if ($image_url) { ?>
        <?php print l(theme('image', $image_url, '', '', null, false), $collection->url, array('html' => true)); ?>
      <?php } ?>
    </div>

    <div class="record">
      <div class="types">
        <?php print $type_list; ?>
      </div>

      <h3>
        <?php print l($collection->title, $collection->url, array('attributes' => array('class' =>'title'))) ;?> 
      </h3>

      <div class="meta">
        <?php if ($collection->creators_string) : ?>
          <span class="creator">
            <?php echo t('By %creator_name%', array('%creator_name%' => $collection->creators_string)) ?>
          </span>
        <?php endif; ?>
        <?php if ($collection->date) : ?>
          <span class="publication_date">
            <?php echo t('(%publication_date%)', array('%publication_date%' => $collection->date)) /* TODO: Improve date handling, localizations etc. */ ?>
          </span>
        <?php endif; ?>
      </div>

      <?php if ($collection->abstract) : ?>
      <div class="abstract">
        <p>
          <?php print check_plain($collection->abstract); ?>
        </p>
      </div>
      <?php endif; ?>

      <?php if ($collection->subjects) : ?>
        <div class="subjects">
          <h4><?php echo t('Subjects:') ?></h4>
          <ul>
            <?php foreach ($collection->subjects as $subject) : ?>
              <li><?php echo $subject ?></li>
            <?php endforeach; ?>
          </ul>
        </div>
      <?php endif; ?>

    </div>

  </li>