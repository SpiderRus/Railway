CREATE TABLE scheme_items_log
(
    item_id     VARCHAR(21) NOT NULL,
    timestamp   TIMESTAMPTZ NOT NULL,
    receive_timestamp TIMESTAMPTZ NOT NULL,
    core        SMALLINT NOT NULL,
    level       SMALLINT NOT NULL,
    tag         VARCHAR(21) NOT NULL,
    message     VARCHAR(260) NOT NULL
);
