-- The database of the blog example. Make it with
--
--     psql "host=localhost dbname=postgres user=postgres" -f examples/blog/schema.sql
--
-- and write it as Saad with the generator into examples/blog/db (see README.md). It is in the schema saad_blog, which
-- this file drops first.

SET client_min_messages TO warning;
DROP SCHEMA IF EXISTS saad_blog CASCADE;
CREATE SCHEMA saad_blog;

CREATE TYPE saad_blog.post_status AS ENUM ('draft', 'published', 'archived');

CREATE TABLE saad_blog.authors (
    id        bigserial PRIMARY KEY,
    name      text NOT NULL,
    email     text NOT NULL UNIQUE,
    bio       text,
    joined_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE saad_blog.posts (
    id           bigserial PRIMARY KEY,
    author_id    bigint NOT NULL REFERENCES saad_blog.authors(id),
    title        text NOT NULL,
    slug         text NOT NULL UNIQUE,
    body         text NOT NULL DEFAULT '',
    status       saad_blog.post_status NOT NULL DEFAULT 'draft',
    views        int NOT NULL DEFAULT 0,
    published_at timestamptz,
    created_at   timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE saad_blog.comments (
    id          bigserial PRIMARY KEY,
    post_id     bigint NOT NULL REFERENCES saad_blog.posts(id) ON DELETE CASCADE,
    author_name text NOT NULL,
    body        text NOT NULL,
    likes       int NOT NULL DEFAULT 0,
    created_at  timestamptz NOT NULL DEFAULT now()
);

-- A table without a key of one column, so its rows are only read and created
CREATE TABLE saad_blog.post_tags (
    post_id bigint NOT NULL REFERENCES saad_blog.posts(id) ON DELETE CASCADE,
    tag     text NOT NULL,
    PRIMARY KEY (post_id, tag)
);
