CREATE TABLE PostHistoryTypes (
  Id SMALLINT NOT NULL,
  Name VARCHAR(50) NOT NULL,
  PRIMARY KEY (Id)
);

CREATE TABLE LinkTypes (
  Id SMALLINT NOT NULL,
  Name VARCHAR(50) NOT NULL,
  PRIMARY KEY (Id)
);

CREATE TABLE PostTypes (
  Id SMALLINT NOT NULL,
  Name VARCHAR(50) NOT NULL,
  PRIMARY KEY (Id)
);

CREATE TABLE CloseReasonTypes (
  Id SMALLINT NOT NULL,
  Name VARCHAR(50) NOT NULL,
  PRIMARY KEY (Id)
);

CREATE TABLE VoteTypes (
  Id SMALLINT NOT NULL,
  Name VARCHAR(50) NOT NULL,
  PRIMARY KEY (Id)
);

CREATE TABLE Users (
  Id INT NOT NULL PRIMARY KEY,
  Reputation INT NOT NULL,
  CreationDate TIMESTAMP NOT NULL,
  DisplayName VARCHAR(40),
  LastAccessDate TIMESTAMP NOT NULL, /* The time when the user last loaded a page; updated every 30 min at most */
  WebsiteUrl VARCHAR(200),
  Location VARCHAR(300),
  AboutMe TEXT,
  Views INT, /* Number of times the profile is viewed */
  UpVotes INT, /* How many upvotes the user has cast */
  DownVotes INT, /* How many downvotes the user has cast */
  ProfileImageUrl VARCHAR(200),
  AccountId INT /* User's Stack Exchange Network profile ID */
);

CREATE TABLE Badges (
  Id INT NOT NULL PRIMARY KEY,
  UserId INT NOT NULL REFERENCES Users (
    Id
  ),
  Name VARCHAR(50) NOT NULL, /* Name of the badge */
  Date TIMESTAMP NOT NULL, /* Date the badge was awarded, e.g. 2008-09-15T08:55:03.923 */
  Class SMALLINT NOT NULL, /* 1 (Gold), 2 (Silver), or 3 (Bronze) */
  TagBased BOOLEAN NOT NULL /* True if badge is for a tag, otherwise it is a named badge */
);

CREATE TABLE Posts (
  Id INT NOT NULL PRIMARY KEY,
  PostTypeId SMALLINT REFERENCES PostTypes (
    Id
  ), /* (listed in the PostTypes table)
    - 1 = Question
    - 2 = Answer
    - 3 = Wiki
    - 4 = TagWikiExcerpt
    - 5 = TagWiki
    - 6 = ModeratorNomination
    - 7 = WikiPlaceholder (Appears to include auxiliary site content like the help center introduction, election description, and the tour page's introduction, ask, and don't ask sections)
    - 8 = PrivilegeWiki */
  AcceptedAnswerId INT, /* only present if PostTypeId = 1 */
  ParentId INT, /* only present if PostTypeId = 2 */
  CreationDate TIMESTAMP,
  Score INT, /* generally non-zero for only Questions, Answers, and Moderator Nominations */
  ViewCount INT,
  Body TEXT, /* as rendered HTML, not Markdown */
  OwnerUserId INT REFERENCES Users (
    Id
  ), /* only present if user has not been deleted; always -1 for tag wiki entries, i.e. the community user owns them */
  OwnerDisplayName VARCHAR(40),
  LastEditorUserId INT REFERENCES Users (
    Id
  ),
  LastEditorDisplayName VARCHAR(40),
  LastEditDate TIMESTAMP, /* e.g. 2009-03-05T22:28:34.823 - the date and time of the most recent edit to the post */
  LastActivityDate TIMESTAMP, /* e.g. 2009-03-11T12:51:01.480 - datetime of the post's most recent activity */
  Title VARCHAR(300), /* question title (PostTypeId = 1), or on Stack Overflow, the tag name for some tag wikis and excerpts (PostTypeId = 4/5) */
  Tags VARCHAR(4000), /* list of question tag (PostTypeId = 1), the tags can be separated using string_to_array(substring(Tags, 2, length(Tags) - 2, '><')), or on Stack Overflow, the subject tag of some tag wikis and excerpts (PostTypeId = 4/5) */
  AnswerCount INT, /* the number of undeleted answers (only present if PostTypeId = 1) */
  CommentCount INT,
  FavoriteCount INT,
  ClosedDate TIMESTAMP, /* present only if the post is closed */
  CommunityOwnedDate TIMESTAMP, /* present only if post is community wiki'd */
  ContentLicense VARCHAR(30)
);

ALTER TABLE Posts ADD   FOREIGN KEY (AcceptedAnswerId) REFERENCES Posts (
    Id
  );

ALTER TABLE Posts ADD   FOREIGN KEY (ParentId) REFERENCES Posts (
    Id
  );

CREATE TABLE Comments (
  Id INT NOT NULL PRIMARY KEY,
  PostId INT NOT NULL REFERENCES Posts (
    Id
  ),
  Score INT,
  Text VARCHAR(2000) NOT NULL, /* Comment body */
  CreationDate TIMESTAMP NOT NULL,
  UserDisplayName VARCHAR(40),
  UserId INT REFERENCES Users (
    Id
  ), /* optional, absent if user has been deleted */
  ContentLicense VARCHAR(30)
);

CREATE TABLE PostHistory (
  Id INT NOT NULL PRIMARY KEY,
  PostHistoryTypeId SMALLINT REFERENCES PostHistoryTypes (
    Id
  ), /* (listed in the PostHistoryTypes table)
    - 1 = Initial Title (initial title (questions only))
    - 2 = Initial Body (initial post raw body text)
    - 3 = Initial Tags (initial list of tags (questions only)
    - 4 = Edit Title (modified title (questions only))
    - 5 = Edit Body (modified post body (raw markdown))
    - 6 = Edit Tags (modified list of tags (questions only))
    - 7 = Rollback Title (reverted title (questions only))
    - 8 = Rollback Body (reverted body (raw markdown))
    - 9 = Rollback Tags (reverted list of tags (questions only))
    - 10 = Post Closed (post voted to be closed)
    - 11 = Post Reopened (post voted to be reopened)
    - 12 = Post Deleted (post voted to be removed)
    - 13 = Post Undeleted (post voted to be restored)
    - 14 = Post Locked (post locked by moderator)
    - 15 = Post Unlocked (post unlocked by moderator)
    - 16 = Community Owned (post now community owned)
    - 17 = Post Migrated (post migrated - now replaced by 35/36 (away/here))
    - 18 = Question Merged (question merged with deleted question)
    - 19 = Question Protected (question was protected by a moderator)
    - 20 = Question Unprotected (question was unprotected by a moderator)
    - 22 = Question Unmerged (answers/votes restored to previously merged question)
    - 24 = Suggested Edit Applied
    - 25 = Post Tweeted
    - 31 = Discussion moved to chat
    - 33 = Post Notice Added (comment contains foreign key to PostNotices)
    - 34 = Post Notice Removed (comment contains foreign key to PostNotices)
    - 35 = Post Migrated Away (replaces id 17)
    - 36 = Post Migrated Here (replaces id 17)
    - 37 = Post Merge Source
    - 38 = Post Merge Destination
    - 50 = CommunityBump (bumped by community user)
    - 52 = SelectedHotQuestion (question became hot network question)
    - 53 = RemovedHotQuestion (question removed from hot network)
    - 66 = CreatedFromWizard */
  PostId INT REFERENCES Posts (
    Id
  ),
  RevisionGUID VARCHAR(36), /* At times more than one type of history record can be recorded by a single action. All of these will be grouped using the same RevisionGUID */
  CreationDate TIMESTAMP,
  UserId INT REFERENCES Users (
    Id
  ),
  UserDisplayName VARCHAR(40), /* populated if a user has been removed and no longer referenced by user id */
  Comment VARCHAR(800), /* This field will contain the comment made by the user who edited a post
    - If PostHistoryTypeId = 10, this field contains the CloseReasonId of the close reason (listed in CloseReasonTypes):
        - Old close reasons:
            - 1 = Exact Duplicate
            - 2 = Off-topic
            - 3 = Subjective and argumentative
            - 4 = Not a real question
            - 7 = Too localized
            - 10 = General reference
            - 20 = Noise or pointless (Meta sites only)
        - Current close reasons:
            - 101 = Duplicate
            - 102 = Off-topic
            - 103 = Needs details or clarity
            - 104 = Needs more focus
            - 105 = Opinion-based
    - If PostHistoryTypeId in (33,34) this field contains the PostNoticeId of the PostNotice */
  Text TEXT, /*  A raw version of the new value for a given revision
    - If PostHistoryTypeId in (10,11,12,13,14,15,19,20,35) this column will contain a JSON encoded string with all users who have voted for the PostHistoryTypeId
    - If it is a duplicate close vote, the JSON string will contain an array of original questions as OriginalQuestionIds
    - If PostHistoryTypeId = 17 this column will contain migration details of either from <url> or to <url> */
  ContentLicense VARCHAR(30)
);

CREATE TABLE PostLinks (
  Id BIGINT NOT NULL PRIMARY KEY,
  CreationDate TIMESTAMP NOT NULL, /* when the link was created */
  PostId INT NOT NULL REFERENCES Posts (
    Id
  ), /* id of source post */
  RelatedPostId INT NOT NULL REFERENCES Posts (
    Id
  ), /* id of target/related post */
  LinkTypeId SMALLINT NOT NULL REFERENCES LinkTypes (
    Id
  ) /* (listed in the LinkTypes table)
    - 1 = Linked (PostId contains a link to RelatedPostId)
    - 3 = Duplicate (PostId is a duplicate of RelatedPostId) */
);

CREATE TABLE Tags (
  Id INT NOT NULL PRIMARY KEY,
  TagName VARCHAR(35),
  Count INT NOT NULL,
  ExcerptPostId INT REFERENCES Posts (
    Id
  ), /* Id of Post that holds the excerpt text of the tag */
  WikiPostId INT REFERENCES Posts (
    Id
  ), /* Id of Post that holds the wiki text of the tag */
  IsModeratorOnly BOOLEAN,
  IsRequired BOOLEAN
);

CREATE TABLE Votes (
  Id INT NOT NULL PRIMARY KEY,
  PostId INT NOT NULL REFERENCES Posts (
    Id
  ),
  VoteTypeId SMALLINT NOT NULL REFERENCES VoteTypes (
    Id
  ), /*  (listed in the VoteTypes table)
    - 1 = AcceptedByOriginator
    - 2 = UpMod (AKA upvote)
    - 3 = DownMod (AKA downvote)
    - 4 = Offensive
    - 5 = Favorite (AKA bookmark; UserId will also be populated) feature removed after October 2022 / replaced by Saves
    - 6 = Close (effective 2013-06-25: Close votes are only stored in table: PostHistory)
    - 7 = Reopen
    - 8 = BountyStart (UserId and BountyAmount will also be populated)
    - 9 = BountyClose (BountyAmount will also be populated)
    - 10 = Deletion
    - 11 = Undeletion
    - 12 = Spam
    - 14 = NominateModerator
    - 15 = ModeratorReview (i.e., a moderator looking at a flagged post)
    - 16 = ApproveEditSuggestion */
  UserId INT REFERENCES Users (
    Id
  ),
  CreationDate TIMESTAMP,
  BountyAmount INT
);
