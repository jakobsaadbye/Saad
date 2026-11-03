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
CREATE TYPE saad_blog.report_status AS ENUM ('open', 'resolved', 'dismissed');

CREATE TABLE saad_blog.authors (
    id        bigserial PRIMARY KEY,
    name      text NOT NULL,
    email     text NOT NULL UNIQUE,
    bio       text,
    joined_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE saad_blog.categories (
    id   bigserial PRIMARY KEY,
    name text NOT NULL,
    slug text NOT NULL UNIQUE
);

CREATE TABLE saad_blog.posts (
    id           bigserial PRIMARY KEY,
    author_id    bigint NOT NULL REFERENCES saad_blog.authors(id),
    category_id  bigint REFERENCES saad_blog.categories(id),
    title        text NOT NULL,
    slug         text NOT NULL UNIQUE,
    body         text NOT NULL DEFAULT '',
    status       saad_blog.post_status NOT NULL DEFAULT 'draft',
    views        int NOT NULL DEFAULT 0,
    published_at timestamptz,
    created_at   timestamptz NOT NULL DEFAULT now()
);

-- The people who read the blog, and log in to the API with their token
CREATE TABLE saad_blog.readers (
    id        bigserial PRIMARY KEY,
    name      text NOT NULL,
    email     text NOT NULL UNIQUE,
    token     text NOT NULL UNIQUE,
    joined_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE saad_blog.comments (
    id          bigserial PRIMARY KEY,
    post_id     bigint NOT NULL REFERENCES saad_blog.posts(id) ON DELETE CASCADE,
    reader_id   bigint REFERENCES saad_blog.readers(id),
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

CREATE TABLE saad_blog.likes (
    id         bigserial PRIMARY KEY,
    reader_id  bigint NOT NULL REFERENCES saad_blog.readers(id),
    post_id    bigint NOT NULL REFERENCES saad_blog.posts(id) ON DELETE CASCADE,
    created_at timestamptz NOT NULL DEFAULT now(),
    UNIQUE (reader_id, post_id)
);

CREATE TABLE saad_blog.follows (
    id         bigserial PRIMARY KEY,
    reader_id  bigint NOT NULL REFERENCES saad_blog.readers(id),
    author_id  bigint NOT NULL REFERENCES saad_blog.authors(id),
    created_at timestamptz NOT NULL DEFAULT now(),
    UNIQUE (reader_id, author_id)
);

CREATE TABLE saad_blog.bookmarks (
    id         bigserial PRIMARY KEY,
    reader_id  bigint NOT NULL REFERENCES saad_blog.readers(id),
    post_id    bigint NOT NULL REFERENCES saad_blog.posts(id) ON DELETE CASCADE,
    note       text,
    created_at timestamptz NOT NULL DEFAULT now(),
    UNIQUE (reader_id, post_id)
);

-- Posts that are read in order, e.g a tutorial in parts
CREATE TABLE saad_blog.series (
    id          bigserial PRIMARY KEY,
    author_id   bigint NOT NULL REFERENCES saad_blog.authors(id),
    title       text NOT NULL,
    slug        text NOT NULL UNIQUE,
    description text NOT NULL DEFAULT ''
);

CREATE TABLE saad_blog.series_posts (
    series_id bigint NOT NULL REFERENCES saad_blog.series(id) ON DELETE CASCADE,
    post_id   bigint NOT NULL REFERENCES saad_blog.posts(id) ON DELETE CASCADE,
    position  int NOT NULL,
    PRIMARY KEY (series_id, post_id)
);

-- Comments that readers report, for a moderator
CREATE TABLE saad_blog.reports (
    id         bigserial PRIMARY KEY,
    comment_id bigint NOT NULL REFERENCES saad_blog.comments(id) ON DELETE CASCADE,
    reader_id  bigint NOT NULL REFERENCES saad_blog.readers(id),
    reason     text NOT NULL,
    status     saad_blog.report_status NOT NULL DEFAULT 'open',
    created_at timestamptz NOT NULL DEFAULT now()
);

-- The newsletter
CREATE TABLE saad_blog.subscribers (
    id         bigserial PRIMARY KEY,
    email      text NOT NULL UNIQUE,
    confirmed  boolean NOT NULL DEFAULT false,
    created_at timestamptz NOT NULL DEFAULT now()
);
