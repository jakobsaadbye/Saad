# A blog backend with web and dbms

A small backend in Saad: a blog with posts, comments, likes, follows, bookmarks, series, moderation and a newsletter.
Its API is its functions: a function with `@web.endpoint` is called at `/v1/<name>` with JSON, and reads and writes
Postgres through the tables that the generator wrote of the schema. It runs as a test in `tools/linux/dbtests.sh`.

```
examples/blog/
├── schema.sql      13 tables in the schema saad_blog
├── db/             written by generate.sd from schema.sql, don't change it
│   ├── database.sd     namespace database: Database, open, a table type per table
│   └── entity/         namespace entities: a struct per table, an enum per Postgres enum
├── api/            namespace api: the endpoints
│   ├── app.sd          the session of a request, the pool, logging in
│   ├── views.sd        what the endpoints answer with
│   ├── posts.sd        listPosts, getPost, createPost, publishPost, feed
│   ├── readers.sd      register, me
│   ├── reactions.sd    likePost, unlikePost, followAuthor, bookmarkPost, listBookmarks
│   ├── comments.sd     addComment, reportComment, openReports, resolveReport
│   ├── catalog.sd      listCategories, getSeries, subscribe, authorStats
│   └── seed.sd         the posts the blog starts with
├── generate.sd     writes db/ from the schema, with postgres.EntityGenerator
├── server.sd       the server
├── demo.sd         a client: starts the server, calls the endpoints and prints the answers
└── tour.sd         a tour of dbms on its own, without the server (below)
```

## Running it

```bash
# 1. The tables (drops the schema saad_blog first)
psql "host=localhost dbname=postgres user=postgres" -f examples/blog/schema.sql

# 2. Write them as Saad into examples/blog/db, with postgres.EntityGenerator. Again when the schema changes
saad run examples/blog/generate.sd -- "host=localhost dbname=postgres user=postgres"

# 3. The server, at http://localhost:8080/v1
saad run examples/blog/server.sd -- "host=localhost dbname=postgres user=postgres" :8080

curl "http://localhost:8080/v1/listPosts?category=history"
curl -X POST -d '{"name": "Charles", "email": "charles@example.com"}' http://localhost:8080/v1/register
curl -X POST -H "Authorization: Bearer <token>" -d '{"slug": "compilers"}' http://localhost:8080/v1/likePost

# Or the demo, which calls every endpoint
saad run examples/blog/demo.sd -- "host=localhost dbname=postgres user=postgres"
```

On Linux, `tools/linux/run.sh` runs the programs instead of `saad run`, with the same arguments but without `--`.

## The endpoints

GET takes its parameters from the query, POST from a JSON object. The ones marked with a lock need
`Authorization: Bearer <token>`, the token of `register`; the ones with a pen need the reader of an author (same email).

| Endpoint | | Parameters | Answer |
| --- | --- | --- | --- |
| `listPosts` | GET | `category?`, `tag?`, `page?` | the published posts, newest first |
| `getPost` | GET | `slug` | the post, its tags and comments. Counts a view |
| `listCategories` | GET | | the categories and how many posts they have |
| `getSeries` | GET | `slug` | a series and its posts, in order |
| `authorStats` | GET | | posts, views and followers of each author |
| `register` | POST | `name`, `email` | the reader and its token |
| `me` 🔒 | GET | | the reader, and how many likes, bookmarks and follows |
| `likePost`, `unlikePost` 🔒 | POST | `slug` | how many like the post |
| `followAuthor` 🔒 | POST | `authorId` | how many follow the author |
| `feed` 🔒 | GET | | the posts of the authors the reader follows |
| `bookmarkPost` 🔒 | POST | `slug`, `note?` | how many bookmarks the reader has |
| `listBookmarks` 🔒 | GET | | the bookmarks |
| `addComment` 🔒 | POST | `slug`, `body` | the comment |
| `reportComment` 🔒 | POST | `commentId`, `reason` | the id of the report |
| `subscribe` | POST | `email` | a text |
| `createPost` 🖊 | POST | `title`, `slug`, `body`, `category?`, `tags` | the draft |
| `publishPost` 🖊 | POST | `slug` | the post |
| `openReports` 🖊 | GET | | the reports that wait |
| `resolveReport` 🖊 | POST | `reportId`, `removeComment` | the report |

Errors are `{"error": "...", "kind": "..."}`: 400 for a parameter that is missing or wrong, 401 without a token,
403 for a reader that isn't an author, 404 for what isn't there and 409 for a slug or an email that is taken.

## How it fits together

An endpoint is a function whose first parameter is the session of the request:

```odin
@web.endpoint(.get)
getSeries :: (session: *Session, slug: string) -> SeriesView {
    series := session.db.series.where(s => s.slug == slug).first();
    if series == null { throw newError(fmt.sprint("No series has the slug '%'", slug), web.NotFound); }
    view := SeriesView{title: series.title, description: series.description, author: series.author().name};
    view.parts = series.seriesPosts()
        .orderBy(s => s.position)
        .select(s => SeriesPart{position: s.position, slug: s.post().slug, title: s.post().title})
        .all();
    return view;
}
```

* **The session** (`api/app.sd`) embeds `web.Session` and has what the endpoints need: a connection of a `dbms.Pool`,
  the tables on it, and the reader of the token. `startSession` and `finishSession` take the connection and give it
  back, for every request.
* **Memory**: while an endpoint runs, what it allocates is in the arena of its request, also what it answers with, so
  there is no `free` in `api/`. It is all freed after the response.
* **Errors**: a query that fails throws, and the server answers with a 500. What the client did wrong is thrown with
  a kind of `web`, e.g `web.NotFound`, which gives its status.
* **Queries** are lambdas made into SQL, with relations as joins (`p.author().name`) and subqueries
  (`p.likes().count()`, `p.postTags().where(t => t.tag == name).exists()`). `feed` is plain SQL with
  `conn.query<PostSummary>`, for a join the tables can't say yet.
* **Changes** to the rows that a query gave are saved by `saveChanges`, e.g the views of `getPost`.

## What the demo prints

```
Reading

GET /v1/listCategories  
    200 [{"name":"History","slug":"history","posts":2},{"name":"Math","slug":"math","posts":1},{"name":"Programming","slug":"programming","posts":1}]

GET /v1/listPosts  
    200 [{"slug":"compilers","title":"Why we need compilers","author":"Grace Hopper","category":"Programming","views":300,"likes":0,"comments":0},{"slug":"first-bug","title":"The first actual bug","author":"Grace Hopper","category":"History","views":210,"likes":0,"comments":0},{"slug":"bernoulli","title":"Computing Bernoulli numbers","author":"Ada Lovelace","category":"Math","views":80,"likes":0,"comments":0},{"slug":"analytical-engine","title":"Notes on the Analytical Engine","author":"Ada Lovelace","category":"History","views":120,"likes":0,"comments":0}]

GET /v1/listPosts?category=history  
    200 [{"slug":"first-bug","title":"The first actual bug","author":"Grace Hopper","category":"History","views":210,"likes":0,"comments":0},{"slug":"analytical-engine","title":"Notes on the Analytical Engine","author":"Ada Lovelace","category":"History","views":120,"likes":0,"comments":0}]

GET /v1/listPosts?tag=math  
    200 [{"slug":"bernoulli","title":"Computing Bernoulli numbers","author":"Ada Lovelace","category":"Math","views":80,"likes":0,"comments":0}]

GET /v1/getPost?slug=compilers  
    200 {"slug":"compilers","title":"Why we need compilers","body":"Why we need compilers","author":"Grace Hopper","category":"Programming","views":301,"likes":0,"tags":["programming"],"comments":[]}

GET /v1/getSeries?slug=from-the-start  
    200 {"title":"Programming, from the start","description":"How programs went from bugs to compilers","author":"Grace Hopper","parts":[{"position":1,"slug":"first-bug","title":"The first actual bug"},{"position":2,"slug":"compilers","title":"Why we need compilers"}]}

GET /v1/getPost?slug=nothing  
    404 {"error":"No post has the slug 'nothing'","kind":"web.NotFound"}
...
```

## The tour of dbms

`tour.sd` is a walkthrough of `std/dbms` without the server, on the tables `authors`, `posts`, `comments` and
`post_tags`:

```bash
saad run examples/blog/tour.sd -- "host=localhost dbname=postgres user=postgres"
```

### 1. The tables as Saad

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
// Generated by dbms.postgres.EntityGenerator from the schema saad_blog of Postgres. Don't change it: generate it again
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

    @Hidden dbLink: dbms.Link;

    // posts_author_id_fkey
    author :: method () -> (*Author, error) {
        return dbms.related<Author>(self.dbLink, "authors", "id", self.authorId);
    }

    // comments_post_id_fkey
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

### 2. Creating rows

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

### 3. Querying

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

### 4. Relations

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

### 5. Changing and removing rows

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

### 6. Plain SQL

What the tables can't say yet, like a join, is plain SQL with `conn.query<T>`, which reads the rows into any struct:

```odin
AuthorStats :: struct { name: string; posts: i64; views: i64; }

stats, err := conn.query<AuthorStats>("SELECT a.name, count(p.id) AS posts, ... GROUP BY a.name", PostStatus.published);
```

### What it prints

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
