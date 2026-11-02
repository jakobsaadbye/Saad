# A blog with dbms

A whole program on Postgres with `std/dbms`. It goes from the schema, to the Saad code that the generator writes of it,
to creating, querying, following relations, changing and removing rows. It runs as a test in `tools/linux/dbtests.sh`.

```
examples/blog/
├── schema.sql              the tables, in the schema saad_blog
├── db/                     written by the generator, don't change it
│   ├── database.sd         namespace database: Database, open, and a type per table
│   └── entity/             namespace entities: a struct per table, an enum per Postgres enum
│       ├── author.sd
│       ├── post.sd
│       ├── post_status.sd
│       ├── comment.sd
│       └── post_tag.sd
└── main.sd                 the app
```

## Running it

```bash
# 1. Make the tables (drops the schema saad_blog first)
psql "host=localhost dbname=postgres user=postgres" -f examples/blog/schema.sql

# 2. Write them as Saad, into examples/blog/db. Do it again when the schema changes
saad run std/dbms/tools/generate.sd -- "host=localhost dbname=postgres user=postgres" examples/blog/db saad_blog

# 3. Run the app
saad run examples/blog/main.sd -- "host=localhost dbname=postgres user=postgres"
```

On Linux, `tools/linux/run.sh` runs the programs instead of `saad run`, with the same arguments but without `--`.
The app empties the tables when it starts, so it can be run again.

## 1. The tables as Saad

`schema.sql` has

```sql
CREATE TYPE saad_blog.post_status AS ENUM ('draft', 'published', 'archived');

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
```

and the generator writes it as a plain struct in `db/entity/post.sd`. A column that can be `NULL` is an optional, the
defaults of the database are the defaults of the struct, and timestamps are `time.Time`. The foreign keys become
methods: `author_id` refers to `authors`, so a `Post` has `author()`, and as `comments` and `post_tags` refer to
`posts`, it has `comments()` and `postTags()`:

```odin
namespace entities;

import dbms;
import time;

Post :: struct {
    id:          i64;
    authorId:    i64;
    title:       string;
    slug:        string;
    body:        string = "";
    status:      PostStatus = .draft;
    views:       i32 = 0;
    publishedAt: ?time.Time;
    createdAt:   time.Time;

    // The database it came from, for author(), comments(), postTags()
    @Hidden dbLink: dbms.Link;

    // The row of authors that authorId refers to, or null
    author :: method () -> (*Author, string) {
        row, err := dbms.related<Author>(self.dbLink, "authors", "id", self.authorId);
        return row, err;
    }

    // The rows of comments whose post_id is the id of this one, as a query, e.g post.comments().all()
    comments :: method () -> dbms.Query<Comment> {
        return dbms.relatedRows<Comment>(self.dbLink, "comments").where(row => row.postId == self.id);
    }
    ...
}

PostStatus :: enum { draft, published, archived }
```

`dbLink` is how a row knows its database. It is `@Hidden`, so it isn't a column, and `fmt` and `json` leave it out.

`db/database.sd` has a table of each, and `open`. The tables with a key of one column are `dbms.KeyedTable`s, which can
also `get`, `update` and `delete` by the key. `post_tags` has a key of two columns, so it is a `dbms.Table`, which can
read and create rows. The generator also says which columns the database makes, so they are read back after a save:

```odin
Database :: struct {
    @embed context: *dbms.Context;   // saveChanges, discardChanges, pendingChanges, ...

    authors:  AuthorsTable;
    comments: CommentsTable;
    postTags: PostTagsTable;
    posts:    PostsTable;
}

open :: (conn: *dbms.Connection) -> Database {
    ...
    tables.posts.table = dbms.keyedTable<Post, i64>(tables.context, "posts", "id", schema: "saad_blog");
    tables.posts.generated = ["id", "created_at"];
    ...
}
```

The app imports both namespaces:

```odin
import database;      // Database and open
use entities;         // Post instead of entities.Post

db := database.open(&conn);
```

## 2. Creating rows

`create()` gives a new row with the defaults. Nothing is written until `saveChanges`, which writes every change in one
transaction, and then what the database made into the rows:

```odin
ada := db.authors.create();
ada.name  = "Ada Lovelace";
ada.email = "ada@example.com";
check(db.saveChanges());
print("% has the id % and joined at %", ada.name, ada.id, ada.joinedAt.format("15:04 on Monday"));

engine := db.posts.create();
engine.authorId    = ada.id;
engine.publishedAt = time.date(2026, 10, 1, 9);     // October 1, 09:00 UTC
```

The creates of one table that come after each other are one `INSERT`, so the four posts of the app are one statement.

## 3. Querying

The lambdas of `where`, `orderBy` and `select` are made into SQL. They are checked like any other code, so a wrong
member or type is a compile error, and the values they use are sent as parameters:

```odin
published, err := db.posts.where(p => p.status == .published).orderByDescending(p => p.publishedAt).all();

// Some columns, into an anonymous struct { title: string; slug: string; popular: bool; }
minViews := 200;
summaries, err2 := db.posts
    .where(p => p.authorId == grace.id)
    .orderBy(p => p.title)
    .select(p => {title: p.title, slug: p.slug, popular: p.views >= minViews})
    .all();

titles, err3 := db.posts.where(p => p.title.toLower().indexOf(term) >= 0).select(p => p.title).all();   // [..]string
recent, err7 := db.posts.where(p => p.publishedAt?.after(since) ?? false).all();    // publishedAt can be NULL
comments, err4 := db.comments.where(c => c.postId == engine.id).count();
taken, err5 := db.posts.where(p => p.slug == "bernoulli").exists();
page, err6 := db.posts.orderBy(p => p.id).offset(1).limit(2).all();
```

The first query is sent as

```sql
SELECT * FROM "saad_blog"."posts" WHERE (("status" = $1)) ORDER BY "published_at" DESC
```

## 4. Relations

The methods of the foreign keys give the row a row refers to, or a query of the rows that refer to it, which is
narrowed and sorted like any other:

```odin
writer, err := moth.author();                                  // a *Author
adaPublished, err2 := ada.posts().where(p => p.status == .published).orderBy(p => p.title).all();
discussion, err3 := engine.comments().orderBy(c => c.id).all();
tags, err4 := engine.postTags().select(t => t.tag).all();     // [..]string
```

The row of `author()` is tracked like a row of `get`. A row that is tracked already is given again instead of a copy,
so `moth.author()` gives the very `*Author` that `create()` gave for Grace, and asking twice gives the same row.

## 5. Changing and removing rows

The rows of `get`, `all` and `first` are tracked: changes to them are saved by `saveChanges`, without calling `update`,
and only the columns that changed are written. The rows of `select` are not tracked.

```odin
post, err := db.posts.where(p => p.slug == "bernoulli").first();   // a *Post, or null when there is none
post.status      = .published;
post.publishedAt = time.now();

db.comments.delete(spam.id);
check(db.saveChanges());      // UPDATE posts SET status = $1, published_at = $2 WHERE id = $3, and the DELETE
```

When a change fails, none of the changes of the save are written, the error says which change it was, and the changes
are kept until they are fixed or `db.discardChanges()`.

## 6. Plain SQL

What the tables can't say yet, like a join, is plain SQL with `conn.query<T>`, which reads the rows into any struct:

```odin
AuthorStats :: struct { name: string; posts: i64; views: i64; }

stats, err := conn.query<AuthorStats>("SELECT a.name, count(p.id) AS posts, ... GROUP BY a.name", PostStatus.published);
```

## What it prints

The times of the rows the database made, like when Ada joined, are those of the run.

```
2 changes to save
Saved authors Ada Lovelace (id 1) and Grace Hopper (id 2)
Ada Lovelace joined at 03:10 on Monday (a time.Time, as the column is a timestamptz)
Saved 4 posts, and comments and tags for them

Published, newest first:
  Why we need compilers (300 views, published Oct 15)
  Notes on the Analytical Engine (120 views, published Oct 1)

Posts of Grace Hopper:
  /first-bug The first actual bug
  /compilers Why we need compilers (popular)

Posts about 'engine': ["Notes on the Analytical Engine", "Computing Bernoulli numbers"]
Published after October 10: ["Why we need compilers"]

'Notes on the Analytical Engine' has 2 comments, and the slug 'bernoulli' is taken: true
Page 2 of the posts, by two: ["bernoulli", "compilers"]

'The first actual bug' is by Grace Hopper
Asking again gives the same row, as it is tracked: true
Published by Ada Lovelace: ["Notes on the Analytical Engine"]
Comments on 'Notes on the Analytical Engine':
  Charles on 'analytical-engine': Brilliant work!
  Mary on 'analytical-engine': When can I see it run?
Tags of 'analytical-engine': ["history", "math"]

2 changes to save: the post that is published, and the view
Drafts left: 0

Removed the spam, 3 comments are left

Saving a second author with the email of Ada fails: create in authors (change 1 of 1): ERROR:  duplicate key value violates unique constraint "authors_email_key"

Published posts per author:
  Ada Lovelace: 2 posts, 121 views
  Grace Hopper: 1 posts, 300 views
```
