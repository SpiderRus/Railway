CREATE TABLE status_log
(
    item_id     VARCHAR(21) NOT NULL,
    timestamp   TIMESTAMPTZ NOT NULL,
    receive_timestamp TIMESTAMPTZ NOT NULL,
    corrected_timestamp TIMESTAMPTZ NOT NULL,
    item_type   VARCHAR(20) NOT NULL,
    internal_temp DECIMAL NOT NULL,
    version     BIGINT NOT NULL,

    marker_timestamp TIMESTAMPTZ,
    speed       SMALLINT,
    power       DECIMAL,

    semaphore_color INTEGER,

    switch_state BOOLEAN
);

CREATE INDEX status_log__corrected__item_id_idx on status_log (corrected_timestamp, item_id);
