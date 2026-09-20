#include "sqlite_storage.h"
//----------------------------------------------------------------------------------------------
#include "global.h"
//----------------------------------------------------------------------------------------------

ASQLiteStorage::ASQLiteStorage () : ASQLiteDatabase(), IAStorage ()
{
	// СѓСЃС‚Р°РЅРѕРІРєР° РєРѕРґР° РїРѕСЃР»РµРґРЅРµР№ РѕС€РёР±РєРё РєР°Рє "РћРљ"
	returnSuccess();
}
//----------------------------------------------------------------------------------------------

ASQLiteStorage::~ASQLiteStorage ()
{
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::returnSuccess ()
{
	IAStorage::setLastError("OK");

	return true;
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::returnError (const QString& message)
{
	if (message == "")
		IAStorage::setLastError("Unknown Error");
	else
		IAStorage::setLastError(message);

	return false;
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::ping ()
{
	QString sql = "SELECT 1 FROM `version`";

	QScopedPointer<AQuery> query_select(createQuery(sql));

	if (query_select.isNull() == true)
		return false;

	return query_select->exec();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::createDatabase ()
{
	QFile file(":sql/avalon.sqlite.sql");

	if (file.open(QIODevice::ReadOnly) == false)
		return returnError(QString::fromUtf8("РћС€РёР±РєР° РѕС‚РєСЂС‹С‚РёСЏ СЂРµСЃСѓСЂСЃР°"));

	QString schema = QString::fromUtf8(file.readAll().data());

	file.close();

	QStringList statements = schema.split(';', Qt::SkipEmptyParts);

	for (int i = 0; i < statements.count(); i++)
	{
		QString sql = statements.at(i).trimmed();

		if (sql.length() == 0)
			continue;

		QScopedPointer<AQuery> query_create(createQuery(sql));

		if (query_create.isNull() == true)
			return returnError(ASQLiteDatabase::getLastError());

		if (query_create->exec() == false)
			return returnError(query_create->getLastError());
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::whoAmI (AUserInfo& info)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`name`,\n";
	sql += "	`nick`,\n";
	sql += "	`real_name`,\n";
	sql += "	`email`,\n";
	sql += "	`homepage`,\n";
	sql += "	`specialization`,\n";
	sql += "	`where_from`,\n";
	sql += "	`origin`\n";
	sql += "FROM\n";
	sql += "	`user`\n";
	sql += "WHERE\n";
	sql += "	`name` = :name";

	QScopedPointer<AQuery> query_select(createQuery(sql));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->bindValue(":name", info.Name.toLower());

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	info.ID = -1;

	while (query_select->next())
	{
		info.ID             = query_select->value(0).toInt();
		info.Name           = query_select->value(1).toString();
		info.Nick           = query_select->value(2).toString();
		info.RealName       = query_select->value(3).toString();
		info.Email          = query_select->value(4).toString();
		info.Homepage       = query_select->value(5).toString();
		info.Specialization = query_select->value(6).toString();
		info.WhereFrom      = query_select->value(7).toString();
		info.Origin         = query_select->value(8).toString();
	}

	if (info.ID == -1)
		return returnError(QString::fromUtf8("РџРѕР»СЊР·РѕРІР°С‚РµР»СЊ ") + info.Name + QString::fromUtf8(" РЅРµ РЅР°Р№РґРµРЅ"));

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getRowVersion (ARowVersion& list)
{
	list.User     = "AAAAAAAAAAA="; // 64С… СЂР°Р·СЂСЏРґРЅРѕРµ С‡РёСЃР»Рѕ 0 РІ РєРѕРґРёСЂРѕРІРєРµ Base64
	list.Message  = "AAAAAAAAAAA=";
	list.Rating   = "AAAAAAAAAAA=";
	list.Moderate = "AAAAAAAAAAA=";

	QString sql;

	sql += "SELECT\n";
	sql += "	`key`,\n";
	sql += "	`value`\n";
	sql += "FROM\n";
	sql += "	`row_version`";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		QString key   = query_select->value(0).toString();
		QString value = query_select->value(1).toString();

		if (key == "message")
			list.Message = value;
		else if (key == "moderate")
			list.Moderate = value;
		else if (key == "rating")
			list.Rating = value;
		else if (key == "user")
			list.User = value;
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getForumList (AForumGroupInfoList& list, bool subscribed_only)
{
	list.clear();

	QString sql;

	sql += "SELECT\n";
	sql += "	`group`.`id`,\n";
	sql += "	`group`.`name`,\n";
	sql += "	`group`.`sort_order`,\n";
	sql += "	`forum`.`id`,\n";
	sql += "	`forum`.`short_name`,\n";
	sql += "	`forum`.`name`,\n";
	sql += "	`forum`.`rated`,\n";
	sql += "	`forum`.`in_top`,\n";
	sql += "	`forum`.`rate_limit`\n";
	sql += "FROM\n";

	if (subscribed_only == true)
	{
		sql += "	`subscribed`\n";
		sql += "		INNER JOIN\n";
		sql += "	`forum`\n";
		sql += "		ON\n";
		sql += "	`subscribed`.`id_forum` = `forum`.`id`\n";
	}
	else
		sql += "	`forum`\n";

	sql += "		INNER JOIN\n";
	sql += "	`group`\n";
	sql += "		ON\n";
	sql += "	`forum`.`id_group` = `group`.`id`\n";
	sql += "ORDER BY\n";
	sql += "	`group`.`sort_order`,\n";
	sql += "	`forum`.`name`";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	int gid_count = -1;
	int gid_old   = -1;

	while (query_select->next())
	{
		int gid_new = query_select->value(0).toInt();

		if (gid_new != gid_old)
		{
			AForumGroupInfo info;

			info.Group.ID        = gid_new;
			info.Group.Name      = query_select->value(1).toString();
			info.Group.SortOrder = query_select->value(2).toInt();

			list.append(info);

			gid_old = gid_new;

			gid_count++;
		}

		AForumInfo forum_info;

		forum_info.ID        = query_select->value(3).toInt();
		forum_info.IDGroup   = gid_new;
		forum_info.ShortName = query_select->value(4).toString();
		forum_info.Name      = query_select->value(5).toString();
		forum_info.Rated     = query_select->value(6).toInt();
		forum_info.InTop     = query_select->value(7).toInt();
		forum_info.RateLimit = query_select->value(8).toInt();

		list[gid_count].Forums.append(forum_info);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::setForumList (const AForumGroupInfoList& list)
{
	if (ASQLiteDatabase::transaction() == false)
		return returnError(ASQLiteDatabase::getLastError());

	QString sql;

	//
	// СѓРґР°Р»РµРЅРёРµ РЅРµСЃСѓС‰РµСЃС‚РІСѓСЋС‰РёС… РіСЂСѓРїРї С„РѕСЂСѓРјРѕРІ
	//

	sql += "DELETE FROM";

	if (list.count() > 0)
	{
		sql += "\n";
		sql += "	`group`\n";
		sql += "WHERE\n";
		sql += "	`id` NOT IN (";

		for (int group_index = 0; group_index < list.count(); group_index++)
		{
			sql += QString::number(list[group_index].Group.ID);

			if (group_index < list.count() - 1)
				sql += ", ";
		}

		sql += ")";
	}
	else
		sql += " `group`";

	QScopedPointer<AQuery> query_delete_group(createQuery(sql, false));

	if (query_delete_group.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_delete_group->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_delete_group->getLastError());
	}

	//
	// СѓРґР°Р»РµРЅРёРµ РЅРµСЃСѓС‰РµСЃС‚РІСѓСЋС‰РёС… С„РѕСЂСѓРјРѕРІ
	//

	sql = "";
	sql += "DELETE FROM";

	if (list.count() > 0)
	{
		// РїРѕР»СѓС‡РµРЅРёРµ РІСЃРµС… id С„РѕСЂСѓРјРѕРІ
		QList<int> ids;

		for (int group_index = 0; group_index < list.count(); group_index++)
			for (int forum_index = 0; forum_index < list[group_index].Forums.count(); forum_index++)
				ids.append(list[group_index].Forums[forum_index].ID);

		// С„РѕСЂРјРёСЂРѕРІР°РЅРёРµ sql
		if (ids.count() > 0)
		{
			sql += "\n";
			sql += "	`forum`\n";
			sql += "WHERE\n";
			sql += "	`id` NOT IN (";

			for (int i = 0; i < ids.count(); i++)
			{
				sql += QString::number(ids[i]);

				if (i < ids.count() - 1)
					sql += ", ";
			}

			sql += ")";
		}
		else
			sql += " `forum`";
	}
	else
		sql += " `forum`";

	QScopedPointer<AQuery> query_delete_forum(createQuery(sql, false));

	if (query_delete_forum.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_delete_forum->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_delete_forum->getLastError());
	}

	//
	// РґРѕР±Р°РІР»РµРЅРёРµ РіСЂСѓРїРї
	//

	if (list.count() > 0)
	{
		sql  = "";
		sql += "REPLACE INTO `group`\n";
		sql += "(\n";
		sql += "	`id`,\n";
		sql += "	`name`,\n";
		sql += "	`sort_order`\n";
		sql += ")\n";
		sql += "VALUES\n";
		sql += "(\n";
		sql += "	:id,\n";
		sql += "	:name,\n";
		sql += "	:sort_order\n";
		sql += ")";

		QScopedPointer<AQuery> query_insert(createQuery(sql));

		if (query_insert.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		for (int i = 0; i < list.count(); i++)
		{
			AGroupInfo info = list[i].Group;

			info.Name.replace("&lt;",  "<");
			info.Name.replace("&gt;",  ">");
			info.Name.replace("&amp;", "&");
			info.Name.replace("\r\n",  " ");
			info.Name.replace("\n",    " ");

			query_insert->bindValue(":id",           info.ID);
			query_insert->bindValue(":name",         info.Name);
			query_insert->bindValue(":sort_order",   info.SortOrder);

			if (query_insert->exec() == false)
			{
				ASQLiteDatabase::rollback();

				return returnError(query_insert->getLastError());
			}
		}
	}

	//
	// РґРѕР±Р°РІР»РµРЅРёРµ С„РѕСЂСѓРјРѕРІ
	//

	if (list.count() > 0)
	{
		AForumInfoList forum_list;

		for (int group_index = 0; group_index < list.count(); group_index++)
			for (int forum_index = 0; forum_index < list[group_index].Forums.count(); forum_index++)
				forum_list.append(list[group_index].Forums[forum_index]);

		if (forum_list.count() > 0)
		{
			sql  = "";
			sql += "REPLACE INTO `forum`\n";
			sql += "(\n";
			sql += "	`id`,\n";
			sql += "	`id_group`,\n";
			sql += "	`short_name`,\n";
			sql += "	`name`,\n";
			sql += "	`rated`,\n";
			sql += "	`in_top`,\n";
			sql += "	`rate_limit`\n";
			sql += ")\n";
			sql += "VALUES\n";
			sql += "(\n";
			sql += "	:id,\n";
			sql += "	:id_group,\n";
			sql += "	:short_name,\n";
			sql += "	:name,\n";
			sql += "	:rated,\n";
			sql += "	:in_top,\n";
			sql += "	:rate_limit\n";
			sql += ")";

			QScopedPointer<AQuery> query_insert(createQuery(sql));

			if (query_insert.isNull() == true)
			{
				QString msg = ASQLiteDatabase::getLastError();

				ASQLiteDatabase::rollback();

				return returnError(msg);
			}

			for (int i = 0; i < forum_list.count(); i++)
			{
				AForumInfo info = forum_list[i];

				info.Name.replace("&lt;",  "<");
				info.Name.replace("&gt;",  ">");
				info.Name.replace("&amp;", "&");
				info.Name.replace("\r\n",  " ");
				info.Name.replace("\n",    " ");

				query_insert->bindValue(":id",           info.ID);
				query_insert->bindValue(":id_group",     info.IDGroup);
				query_insert->bindValue(":short_name",   info.ShortName);
				query_insert->bindValue(":name",         info.Name);
				query_insert->bindValue(":rated",        (info.Rated == true ? 1 : 0));
				query_insert->bindValue(":in_top",       (info.InTop == true ? 1 : 0));
				query_insert->bindValue(":rate_limit",   info.RateLimit);

				if (query_insert->exec() == false)
				{
					ASQLiteDatabase::rollback();

					return returnError(query_insert->getLastError());
				}
			}
		}
	}

	//
	// Р·Р°РІРµСЂС€РµРЅРёРµ С‚СЂР°РЅР·Р°РєС†РёРё
	//

	if (ASQLiteDatabase::commit() == false)
		return returnError(ASQLiteDatabase::getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getSubscribedForumList (ASubscribedForumInfoList& list)
{
	list.clear();

	QString sql;

	sql += "SELECT\n";
	sql += "	`id_forum`,\n";
	sql += "	`is_first`\n";
	sql += "FROM\n";
	sql += "	`subscribed`";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		ASubscribedForumInfo info;

		info.IDForum = query_select->value(0).toInt();
		info.IsFirst = query_select->value(1).toInt();

		list.append(info);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::setSubscribedForumList (const ASubscribedForumInfoList& list)
{
	if (ASQLiteDatabase::transaction() == false)
		return returnError(ASQLiteDatabase::getLastError());

	QString sql;

	//
	// СѓРґР°Р»РµРЅРёРµ С„РѕСЂСѓРјРѕРІ, РѕС‚ РєРѕС‚РѕСЂС‹С… РѕС‚РїРёСЃР°Р»РёСЃСЊ
	//

	sql += "DELETE FROM";

	if (list.count() > 0)
	{
		sql += "\n";
		sql += "	`subscribed`\n";
		sql += "WHERE\n";
		sql += "	`id_forum` NOT IN (";

		for (int i = 0; i < list.count(); i++)
		{
			sql += QString::number(list[i].IDForum);

			if (i < list.count() - 1)
				sql += ", ";
		}

		sql += ")";
	}
	else
		sql += " `subscribed`";

	QScopedPointer<AQuery> query_delete(createQuery(sql, false));

	if (query_delete.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_delete->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_delete->getLastError());
	}

	//
	// РІСЃС‚Р°РІРєР° С„РѕСЂСѓРјРѕРІ, РЅР° РєРѕС‚РѕСЂС‹Рµ РїРѕРґРїРёСЃР°Р»РёСЃСЊ
	//

	if (list.count() > 0)
	{
		sql  = "";
		sql += "INSERT INTO `subscribed`\n";
		sql += "(\n";
		sql += "	`id_forum`,\n";
		sql += "	`is_first`\n";
		sql += ")\n";
		sql += "VALUES\n";
		sql += "(\n";
		sql += "	:id_forum,\n";
		sql += "	1\n";
		sql += ")";

		QScopedPointer<AQuery> query_insert(createQuery(sql));

		if (query_insert.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		sql  = "";
		sql += "SELECT\n";
		sql += "	COUNT(*)\n";
		sql += "FROM\n";
		sql += "	`subscribed`\n";
		sql += "WHERE\n";
		sql += "	`id_forum` = :id_forum";

		QScopedPointer<AQuery> query_exists(createQuery(sql));

		if (query_exists.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		for (int i = 0; i < list.count(); i++)
		{
			ASubscribedForumInfo info = list.at(i);

			query_exists->bindValue(":id_forum", info.IDForum);

			if (query_exists->exec() == false)
			{
				ASQLiteDatabase::rollback();

				return returnError(query_exists->getLastError());
			}

			query_exists->next();

			bool exists = query_exists->value(0).toInt();

			if (exists == false)
			{
				query_insert->bindValue(":id_forum", info.IDForum);

				if (query_insert->exec() == false)
				{
					ASQLiteDatabase::rollback();

					return returnError(query_insert->getLastError());
				}
			}
		}
	}

	//
	// Р·Р°РІРµСЂС€РµРЅРёРµ С‚СЂР°РЅР·Р°РєС†РёРё
	//

	if (ASQLiteDatabase::commit() == false)
		return returnError(ASQLiteDatabase::getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getForumInfo (int id_forum, AForumInfo& info)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`id_group`,\n";
	sql += "	`short_name`,\n";
	sql += "	`name`,\n";
	sql += "	`rated`,\n";
	sql += "	`in_top`,\n";
	sql += "	`rate_limit`\n";
	sql += "FROM\n";
	sql += "	`forum`\n";
	sql += "WHERE\n";
	sql += "	`id` = :id";

	QScopedPointer<AQuery> query_select(createQuery(sql));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->bindValue(":id", id_forum);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		info.ID        = query_select->value(0).toInt();
		info.IDGroup   = query_select->value(1).toInt();
		info.ShortName = query_select->value(2).toString();
		info.Name      = query_select->value(3).toString();
		info.Rated     = query_select->value(4).toInt();
		info.InTop     = query_select->value(5).toInt();
		info.RateLimit = query_select->value(6).toInt();
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::setUserList (const AUserInfoList& list, const QString& row_version)
{
	if (ASQLiteDatabase::transaction() == false)
		return returnError(ASQLiteDatabase::getLastError());

	QString sql;

	//
	// СЃРѕС…СЂР°РЅРµРЅРёРµ РІРµСЂСЃРёРё СЃРїРёСЃРєР°
	//

	sql += "REPLACE INTO `row_version`\n";
	sql += "(\n";
	sql += "	`key`,\n";
	sql += "	`value`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:key,\n";
	sql += "	:value\n";
	sql += ")";

	QScopedPointer<AQuery> query_insert_row(createQuery(sql));

	if (query_insert_row.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	query_insert_row->bindValue(":key",   "user");
	query_insert_row->bindValue(":value", row_version);

	if (query_insert_row->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_insert_row->getLastError());
	}

	//
	// СЃРѕС…СЂР°РЅРµРЅРёРµ РїРѕР»СЊР·РѕРІР°С‚РµР»РµР№
	//

	sql  = "";
	sql += "REPLACE INTO `user`\n";
	sql += "(\n";
	sql += "	`id`,\n";
	sql += "	`name`,\n";
	sql += "	`nick`,\n";
	sql += "	`real_name`,\n";
	sql += "	`email`,\n";
	sql += "	`homepage`,\n";
	sql += "	`specialization`,\n";
	sql += "	`where_from`,\n";
	sql += "	`origin`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id,\n";
	sql += "	:name,\n";
	sql += "	:nick,\n";
	sql += "	:real_name,\n";
	sql += "	:email,\n";
	sql += "	:homepage,\n";
	sql += "	:specialization,\n";
	sql += "	:where_from,\n";
	sql += "	:origin\n";
	sql += ")";

	QScopedPointer<AQuery> query_insert(createQuery(sql));

	if (query_insert.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	for (int i = 0; i < list.count(); i++)
	{
		AUserInfo info = list[i];

		query_insert->bindValue(":id",             info.ID);
		query_insert->bindValue(":name",           info.Name.toLower());
		query_insert->bindValue(":nick",           info.Nick);
		query_insert->bindValue(":real_name",      info.RealName);
		query_insert->bindValue(":email",          info.Email);
		query_insert->bindValue(":homepage",       info.Homepage);
		query_insert->bindValue(":specialization", info.Specialization);
		query_insert->bindValue(":where_from",     info.WhereFrom);
		query_insert->bindValue(":origin",         info.Origin);

		if (query_insert->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_insert->getLastError());
		}
	}

	//
	// Р·Р°РІРµСЂС€РµРЅРёРµ С‚СЂР°РЅР·Р°РєС†РёРё
	//

	if (ASQLiteDatabase::commit() == false)
		return returnError(ASQLiteDatabase::getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMaxIDMessage (int& max_id)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	MAX(`id`)\n";
	sql += "FROM\n";
	sql += "	`message`";

	QScopedPointer<AQuery> query(createQuery(sql, false));

	if (query.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query->exec() == false)
		return returnError(query->getLastError());

	query->next();

	max_id = query->value(0).toInt();

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMinIDMessage (int& min_id)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	MIN(`id`)\n";
	sql += "FROM\n";
	sql += "	`message`";

	QScopedPointer<AQuery> query(createQuery(sql, false));

	if (query.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query->exec() == false)
		return returnError(query->getLastError());

	query->next();

	min_id = query->value(0).toInt();

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMessageQuery (ADataQuery& query)
{
	query.Forum.clear();
	query.BrokenMessage.clear();
	query.BrokenTopic.clear();

	QString sql;

	//
	// СЃРїРёСЃРѕРє РїРѕРґРїРёСЃР°РЅС‹С… С„РѕСЂСѓРјРѕРІ
	//

	sql += "SELECT\n";
	sql += "	`id_forum`,\n";
	sql += "	`is_first`\n";
	sql += "FROM\n";
	sql += "	`subscribed`";

	QScopedPointer<AQuery> query_subscribed_select(createQuery(sql, false));

	if (query_subscribed_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_subscribed_select->setForwardOnly(true);

	if (query_subscribed_select->exec() == false)
		return returnError(query_subscribed_select->getLastError());

	while (query_subscribed_select->next())
	{
		ASubscribedForumInfo info;

		info.IDForum = query_subscribed_select->value(0).toInt();
		info.IsFirst = query_subscribed_select->value(1).toInt();

		if (info.IDForum != 0)
			query.Forum.append(info);
	}

	//
	// СЃРїРёСЃРѕРє РїРѕР»РѕРјР°РЅРЅС‹С… СЃРѕРѕР±С‰РµРЅРёР№
	//

	// Р—Р° РІСЂРµРјСЏ РїСЂРѕС€РµРґС€РµРµ СЃ РїСЂРµРґС‹РґСѓС‰РµР№ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё СЃРѕРѕР±С‰РµРЅРёРµ РјРѕРіР»Рё РѕС‚РґРµР»РёС‚СЊ
	// Рё РІРѕРѕР±С‰Рµ РїРµСЂРµРЅРµСЃС‚Рё РІ РґСЂСѓРіРѕР№ С„РѕСЂСѓРј, РїРѕСЌС‚РѕРјСѓ parentId РјРѕР¶РµС‚ Р±С‹С‚СЊ РЅРµ Р°РєС‚СѓР°Р»РµРЅ.
	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_message`\n";
	sql += "FROM\n";
	sql += "	`broken`\n";
	sql += "WHERE\n";
	sql += "	`is_topic` = 0";

	QScopedPointer<AQuery> query_broken_message_select(createQuery(sql, false));

	if (query_broken_message_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_broken_message_select->setForwardOnly(true);

	if (query_broken_message_select->exec() == false)
		return returnError(query_broken_message_select->getLastError());

	while (query_broken_message_select->next())
	{
		int id = query_broken_message_select->value(0).toInt();

		query.BrokenMessage.append(id);
	}

	//
	// СЃРїРёСЃРѕРє РїРѕР»РѕРјР°РЅРЅС‹С… С‚РѕРїРёРєРѕРІ
	//

	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_message`\n";
	sql += "FROM\n";
	sql += "	`broken`\n";
	sql += "WHERE\n";
	sql += "	`is_topic` > 0";

	QScopedPointer<AQuery> query_broken_topic_select(createQuery(sql, false));

	if (query_broken_topic_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_broken_topic_select->setForwardOnly(true);

	if (query_broken_topic_select->exec() == false)
		return returnError(query_broken_topic_select->getLastError());

	while (query_broken_topic_select->next())
	{
		int id = query_broken_topic_select->value(0).toInt();

		query.BrokenTopic.append(id);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::setMessageList (const ADataList& list, const ARowVersion& row_version, bool save_row_version)
{
	if (ASQLiteDatabase::transaction() == false)
		return returnError(ASQLiteDatabase::getLastError());

	#ifdef AVALON_USE_ZLIB
	AGlobal* global = AGlobal::getInstance();
	#endif

	QString sql;

	//
	// СЃРѕС…СЂР°РЅРµРЅРёРµ РІРµСЂСЃРёР№
	//

	if (save_row_version == true)
	{
		sql += "REPLACE INTO `row_version`\n";
		sql += "(\n";
		sql += "	`key`,\n";
		sql += "	`value`\n";
		sql += ")\n";
		sql += "VALUES\n";
		sql += "(\n";
		sql += "	:key,\n";
		sql += "	:value\n";
		sql += ")";

		QScopedPointer<AQuery> query_insert_row(createQuery(sql));

		if (query_insert_row.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		query_insert_row->bindValue(":key",   "message");
		query_insert_row->bindValue(":value", row_version.Message);

		if (query_insert_row->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_insert_row->getLastError());
		}

		query_insert_row->bindValue(":key",   "rating");
		query_insert_row->bindValue(":value", row_version.Rating);

		if (query_insert_row->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_insert_row->getLastError());
		}

		query_insert_row->bindValue(":key",   "moderate");
		query_insert_row->bindValue(":value", row_version.Moderate);

		if (query_insert_row->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_insert_row->getLastError());
		}
	}

	//
	// СЃРѕС…СЂР°РЅРµРЅРёРµ СЃРѕРѕР±С‰РµРЅРёР№
	//

	// Р·Р°РїСЂРѕСЃ РґР»СЏ РІСЃС‚Р°РІРєРё СЃРѕРѕР±С‰РµРЅРёР№
	sql  = "";
	sql += "REPLACE INTO `message`\n";
	sql += "(\n";
	sql += "	`id`,\n";
	sql += "	`id_topic`,\n";
	sql += "	`id_parent`,\n";
	sql += "	`id_user`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`subject`,\n";
	sql += "	`message_name`,\n";
	sql += "	`user_nick`,\n";
	sql += "	`message`,\n";
	sql += "	`id_article`,\n";
	sql += "	`message_date`,\n";
	sql += "	`update_date`,\n";
	sql += "	`user_role`,\n";
	sql += "	`user_title`,\n";
	sql += "	`user_title_color`,\n";
	sql += "	`last_moderated`,\n";
	sql += "	`closed`,\n";
	sql += "	`has_child`,\n";
	sql += "	`compressed`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id,\n";
	sql += "	:id_topic,\n";
	sql += "	:id_parent,\n";
	sql += "	:id_user,\n";
	sql += "	:id_forum,\n";
	sql += "	:subject,\n";
	sql += "	:message_name,\n";
	sql += "	:user_nick,\n";
	sql += "	:message,\n";
	sql += "	:id_article,\n";
	sql += "	:message_date,\n";
	sql += "	:update_date,\n";
	sql += "	:user_role,\n";
	sql += "	:user_title,\n";
	sql += "	:user_title_color,\n";
	sql += "	:last_moderated,\n";
	sql += "	:closed,\n";
	sql += "	0,\n";
	sql += "	:compressed\n";
	sql += ")";

	QScopedPointer<AQuery> query_insert_message(createQuery(sql));

	if (query_insert_message.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	// Р·Р°РїСЂРѕСЃ РґР»СЏ РІСЃС‚Р°РІРєРё РІ СЃРїРёСЃРѕРє С‚РѕРїРёРєРѕРІ
	sql  = "";
	sql += "REPLACE INTO `message_topic`\n";
	sql += "(\n";
	sql += "	`id_message`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`message_date`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id_message,\n";
	sql += "	:id_forum,\n";
	sql += "	:message_date\n";
	sql += ")";

	QScopedPointer<AQuery> query_insert_message_topic(createQuery(sql));

	if (query_insert_message_topic.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	// Р·Р°РїСЂРѕСЃ РґР»СЏ РІСЃС‚Р°РІРєРё РІ СЃРїРёСЃРѕРє РЅРµРїСЂРѕС‡РёС‚Р°РЅРЅС‹С… СЃРѕРѕР±С‰РµРЅРёР№
	sql  = "";
	sql += "REPLACE INTO `unread`\n";
	sql += "(\n";
	sql += "	`id_message`,\n";
	sql += "	`id_parent`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`id_topic`,\n";
	sql += "	`id_parent_user`,\n";
	sql += "	`message_date`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id_message,\n";
	sql += "	:id_parent,\n";
	sql += "	:id_forum,\n";
	sql += "	:id_topic,\n";
	sql += "	0,\n";
	sql += "	:message_date\n";
	sql += ")";

	QScopedPointer<AQuery> query_insert_unread(createQuery(sql));

	if (query_insert_unread.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	#ifdef AVALON_USE_ZLIB
	// Р·Р°РіРѕС‚РѕРІРєР° РїР°РјСЏС‚Рё РґР»СЏ СЃР¶Р°С‚РёСЏ СЃРѕРѕР±С‰РµРЅРёР№
	uLongf compressed_buf_size      = 3 * 1024 * 1024; // СЂР°Р·РјРµСЂ Р±СѓС„РµСЂР° РґР»СЏ СЃР¶Р°С‚С‹С… РґР°РЅРЅС‹С… (РјРѕР¶РµС‚ РјРµРЅСЏС‚СЊСЃСЏ РїСЂРё РЅРµРѕР±С…РѕРґРёРјРѕСЃС‚Рё)
	Bytef* compressed_buf           = 0;               // Р±СѓС„РµСЂ РґР»СЏ СЃР¶Р°С‚С‹С… РґР°РЅРЅС‹С…
	uLongf compressed_buf_size_real = 0;               // СЂРµР°Р»СЊРЅР°СЏ РґР»РёРЅР° СЃР¶Р°С‚С‹С… РґР°РЅРЅС‹С…

	if (global->Compression == true)
		compressed_buf = new Bytef[compressed_buf_size];
	#endif

	// СЃРїРёСЃРѕРє РґР»СЏ С„Р»Р°РіРѕРІ РЅР°Р»РёС‡РёСЏ РґРѕС‡РµСЂРЅРёС… СЃРѕРѕР±С‰РµРЅРёР№
	QList<int> check_parent;
	QList<int> check_child;

	for (int i = 0; i < list.Message.count(); i++)
	{
		AMessageInfo info = list.Message[i];

		#ifdef AVALON_USE_ZLIB
		// С„Р»Р°Рі С‚РѕРіРѕ, С‡С‚Рѕ РЅСѓР¶РЅРѕ СЃРѕС…СЂР°РЅСЏС‚СЊ СЃР¶Р°С‚С‹Р№ РІР°СЂРёР°РЅС‚ СЃРѕРѕР±С‰РµРЅРёСЏ
		bool compressed = false;

		// РїСЂРѕРІРµСЂРєР° РЅРµРѕР±С…РѕРґРёРјРѕСЃС‚Рё СЃР¶Р°С‚РёСЏ
		if (global->Compression == true)
		{
			QByteArray message = info.Message.toUtf8();

			uLongf       source_buf_size = message.count();
			const Bytef* source_buf      = (const Bytef*)message.constData();

			// РїСЂРѕРІРµСЂРєР° РЅР° РґРѕСЃС‚Р°С‚РѕС‡РЅСѓСЋ СЂР°Р·РјРµСЂРЅРѕСЃС‚СЊ Р±СѓС„РµСЂР° СЃР¶Р°С‚РёСЏ
			if (source_buf_size * 2 > compressed_buf_size)
			{
				delete [] compressed_buf;
				compressed_buf_size = source_buf_size * 2;
				compressed_buf = new Bytef[compressed_buf_size];
			}

			compressed_buf_size_real = compressed_buf_size;

			// СЃР¶Р°С‚РёРµ (СЃРј. РґРѕРєСѓРјРµРЅС‚Р°С†РёСЋ РїРѕ zlib)
			int z_result = compress2(compressed_buf, &compressed_buf_size_real, source_buf, source_buf_size, 9);

			// Р°РЅР°Р»РёР· СЂРµР·СѓР»СЊС‚Р°С‚Р°, РµСЃР»Рё РїСЂРѕРёР·РѕС€Р»Р° РѕС€РёР±РєР° РёР»Рё СЂР°Р·РјРµСЂ СЃР¶Р°С‚С‹С… РґР°РЅРЅС‹С… Р±РѕР»СЊС€Рµ РёСЃС…РѕРґРЅС‹С…
			if (z_result != Z_OK)
				compressed = false;
			else if (compressed_buf_size_real >= source_buf_size)
				compressed = false;
			else
				compressed = true;
		}
		#endif

		// РІСЃС‚Р°РІРєР° СЃРѕРѕР±С‰РµРЅРёР№
		if (info.IDParent != 0)
		{
			if (check_parent.contains(info.IDParent) == false)
				check_parent.append(info.IDParent);
		}

		check_child.append(info.ID);

		info.Subject.replace("&lt;",  "<");
		info.Subject.replace("&gt;",  ">");
		info.Subject.replace("&amp;", "&");
		info.Subject.replace("\r\n",  " ");
		info.Subject.replace("\n",    " ");

		query_insert_message->bindValue(":id",           info.ID);
		query_insert_message->bindValue(":id_topic",     info.IDTopic);
		query_insert_message->bindValue(":id_parent",    info.IDParent);
		query_insert_message->bindValue(":id_user",      info.IDUser);
		query_insert_message->bindValue(":id_forum",     info.IDForum);
		query_insert_message->bindValue(":subject",      info.Subject);
		query_insert_message->bindValue(":message_name", info.MessageName);
		query_insert_message->bindValue(":user_nick",    info.UserNick);

		#ifdef AVALON_USE_ZLIB
		if (compressed == false)
		#endif
			query_insert_message->bindValue(":message", info.Message);
		#ifdef AVALON_USE_ZLIB
		else
			query_insert_message->bindValue(":message", QByteArray(reinterpret_cast<const char*>(compressed_buf), compressed_buf_size_real));
		#endif

		query_insert_message->bindValue(":id_article",       info.IDArticle);
		query_insert_message->bindValue(":message_date",     info.MessageDate.toSecsSinceEpoch());
		query_insert_message->bindValue(":update_date",      info.UpdateDate.toSecsSinceEpoch());
		query_insert_message->bindValue(":user_role",        info.UserRole);
		query_insert_message->bindValue(":user_title",       info.UserTitle);
		query_insert_message->bindValue(":user_title_color", info.UserTitleColor);
		query_insert_message->bindValue(":last_moderated",   info.LastModerated.toSecsSinceEpoch());
		query_insert_message->bindValue(":closed",           (info.Closed == true ? 1 : 0));

		#ifdef AVALON_USE_ZLIB
		query_insert_message->bindValue(":compressed",       (compressed  == true ? 1 : 0));
		#else
		query_insert_message->bindValue(":compressed",       0);
		#endif

		if (query_insert_message->exec() == false)
		{
			#ifdef AVALON_USE_ZLIB
			// РѕС‡РёСЃС‚РєР° Р±СѓС„РµСЂР° РєРѕРјРїСЂРµСЃСЃРёРё
			delete [] compressed_buf;
			#endif

			ASQLiteDatabase::rollback();

			return returnError(query_insert_message->getLastError());
		}

		// СЃРѕРѕР±С‰РµРЅРёСЏ СЃ РЅСѓР»РµРІС‹Рј С„РѕСЂСѓРјРѕРј РІ СЃРїРёСЃРєРµ С‚РѕРїРёРєРѕРІ РЅРµ С‚СЂРµР±СѓСЋС‚СЃСЏ
		if (info.IDTopic == 0 && info.IDForum != 0)
		{
			query_insert_message_topic->bindValue(":id_message",     info.ID);
			query_insert_message_topic->bindValue(":id_forum",       info.IDForum);
			query_insert_message_topic->bindValue(":message_date",   info.MessageDate.toSecsSinceEpoch());

			if (query_insert_message_topic->exec() == false)
			{
				#ifdef AVALON_USE_ZLIB
				// РѕС‡РёСЃС‚РєР° Р±СѓС„РµСЂР° РєРѕРјРїСЂРµСЃСЃРёРё
				delete [] compressed_buf;
				#endif

				ASQLiteDatabase::rollback();

				return returnError(query_insert_message_topic->getLastError());
			}
		}

		// СЃС‚СЂР°РЅРЅРѕ, РЅРѕ Р±С‹РІР°СЋС‚ СЃРѕРѕР±С‰РµРЅРёСЏ СЃ РЅСѓР»РµРІС‹Рј С„РѕСЂСѓРјРѕРј,
		// РєРѕС‚РѕСЂС‹Рµ РїРѕС‚РѕРј Р±СѓРґСѓС‚ РІРµС‡РЅРѕ С…СЂР°РЅРёС‚СЊСЃСЏ РІ unread
		if (info.IDForum != 0 && save_row_version == true && query_insert_message->numRowsAffected() > 0)
		{
			query_insert_unread->bindValue(":id_message",     info.ID);
			query_insert_unread->bindValue(":id_parent",      info.IDParent);
			query_insert_unread->bindValue(":id_forum",       info.IDForum);
			query_insert_unread->bindValue(":id_topic",       info.IDTopic);
			query_insert_unread->bindValue(":message_date",   info.MessageDate.toSecsSinceEpoch());

			if (query_insert_unread->exec() == false)
			{
				#ifdef AVALON_USE_ZLIB
				// РѕС‡РёСЃС‚РєР° Р±СѓС„РµСЂР° РєРѕРјРїСЂРµСЃСЃРёРё
				delete [] compressed_buf;
				#endif

				ASQLiteDatabase::rollback();

				return returnError(query_insert_unread->getLastError());
			}
		}
	}

	#ifdef AVALON_USE_ZLIB
	// РѕС‡РёСЃС‚РєР° Р±СѓС„РµСЂР° РєРѕРјРїСЂРµСЃСЃРёРё
	delete [] compressed_buf;
	#endif

	//
	// РѕР±РЅРѕРІР»РµРЅРёРµ СЃРїРёСЃРєР° СЂРѕРґРёС‚РµР»СЊСЃРєРёС… РїРѕР»СЊР·РѕРІР°С‚РµР»РµР№ РІ СЃРїРёСЃРєРµ РЅРµРїСЂРѕС‡РёС‚Р°РЅРЅС‹С… СЃРѕРѕР±С‰РµРЅРёР№
	//

	sql  = "";
	sql += "UPDATE\n";
	sql += "	`unread`\n";
	sql += "SET\n";
	sql += "	`id_parent_user` = IFNULL(\n";
	sql += "		(\n";
	sql += "			SELECT\n";
	sql += "				IFNULL(`message`.`id_user`, 0)\n";
	sql += "			FROM\n";
	sql += "				`message`\n";
	sql += "			WHERE\n";
	sql += "				`message`.`id` = `unread`.`id_parent`\n";
	sql += "		),\n";
	sql += " 		0\n";
	sql += "	)\n";
	sql += "WHERE\n";
	sql += "	`id_parent` <> 0";

	QScopedPointer<AQuery> query_update_unread(createQuery(sql, false));

	if (query_update_unread.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_update_unread->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_update_unread->getLastError());
	}

	//
	// РѕР±РЅРѕРІР»РµРЅРёРµ РјР°СЂРєРµСЂРѕРІ РґРѕС‡РµСЂРЅРёС… СЃРѕРѕР±С‰РµРЅРёР№
	//

	if (check_parent.count() > 0)
	{
		sql  = "";
		sql += "UPDATE\n";
		sql += "	`message`\n";
		sql += "SET\n";
		sql += "	`has_child` = 1\n";
		sql += "WHERE\n";
		sql += "	`id` IN (";

		for (int i = 0; i < check_parent.count(); i++)
		{
			sql += QString::number(check_parent[i]);

			if (i < check_parent.count() - 1)
				sql += ", ";
		}

		sql += ")";

		QScopedPointer<AQuery> query_update_child(createQuery(sql, false));

		if (query_update_child.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		if (query_update_child->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_update_child->getLastError());
		}
	}

	if (check_child.count() > 0)
	{
		sql  = "";
		sql += "SELECT\n";
		sql += "	COUNT(*)\n";
		sql += "FROM\n";
		sql += "	`message`\n";
		sql += "WHERE\n";
		sql += "	`id_parent` = :id_parent\n";
		sql += "LIMIT 1";

		QScopedPointer<AQuery> query_select_child(createQuery(sql));

		if (query_select_child.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		sql  = "";
		sql += "UPDATE\n";
		sql += "	`message`\n";
		sql += "SET\n";
		sql += "	`has_child` = :has_child\n";
		sql += "WHERE\n";
		sql += "	`id` = :id";

		QScopedPointer<AQuery> query_update_child(createQuery(sql));

		if (query_update_child.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		for (int i = 0; i < check_child.count(); i++)
		{
			query_select_child->bindValue(":id_parent", check_child[i]);

			if (query_select_child->exec() == false)
			{
				ASQLiteDatabase::rollback();

				return returnError(query_select_child->getLastError());
			}

			query_select_child->next();

			bool flag = query_select_child->value(0).toInt();

			query_update_child->bindValue(":id",        check_child[i]);
			query_update_child->bindValue(":has_child", (flag == true ? 1 : 0));

			if (query_update_child->exec() == false)
			{
				ASQLiteDatabase::rollback();

				return returnError(query_update_child->getLastError());
			}
		}
	}

	//
	// СЃРѕС…СЂР°РЅРµРЅРёРµ СЂРµР№С‚РёРЅРіР°
	//

	sql  = "";
	sql += "REPLACE INTO `rating`\n";
	sql += "(\n";
	sql += "	`id_message`,\n";
	sql += "	`id_topic`,\n";
	sql += "	`id_user`,\n";
	sql += "	`user_rating`,\n";
	sql += "	`rate`,\n";
	sql += "	`rate_date`,\n";
	sql += "	`rate_type`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id_message,\n";
	sql += "	:id_topic,\n";
	sql += "	:id_user,\n";
	sql += "	:user_rating,\n";
	sql += "	:rate,\n";
	sql += "	:rate_date,\n";
	sql += "	:rate_type\n";
	sql += ")";

	QScopedPointer<AQuery> query_insert_rating(createQuery(sql));

	if (query_insert_rating.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	for (int i = 0; i < list.Rating.count(); i++)
	{
		ARatingInfo info = list.Rating[i];

		query_insert_rating->bindValue(":id_message",    info.IDMessage);
		query_insert_rating->bindValue(":id_topic",      info.IDTopic);
		query_insert_rating->bindValue(":id_user",       info.IDUser);
		query_insert_rating->bindValue(":user_rating",   info.UserRating);
		query_insert_rating->bindValue(":rate",          info.Rate);
		query_insert_rating->bindValue(":rate_date",     info.RateDate.toSecsSinceEpoch());

		// "+1" = -3, "1" = 1, "2" = 2, "3" = 3, "+" = -4, "-" = 0, ";)" = -2
		if (info.Rate == -2)
			query_insert_rating->bindValue(":rate_type", SPECIAL_RATE_TYPE_SMILE);
		else if (info.Rate == -4 || info.Rate == 0)
			query_insert_rating->bindValue(":rate_type", SPECIAL_RATE_TYPE_PLUS_MINUS);
		else
			query_insert_rating->bindValue(":rate_type", SPECIAL_RATE_TYPE_NUMBER);

		if (query_insert_rating->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_insert_rating->getLastError());
		}
	}

	//
	// РјРѕРґРµСЂРёР»РєРё
	//

	sql  = "";
	sql += "REPLACE INTO `moderate`\n";
	sql += "(\n";
	sql += "	`id_message`,\n";
	sql += "	`id_topic`,\n";
	sql += "	`id_user`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`created`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id_message,\n";
	sql += "	:id_topic,\n";
	sql += "	:id_user,\n";
	sql += "	:id_forum,\n";
	sql += "	:created\n";
	sql += ")";

	QScopedPointer<AQuery> query_insert_moderate(createQuery(sql));

	if (query_insert_moderate.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	for (int i = 0; i < list.Moderate.count(); i++)
	{
		AModerateInfo info = list.Moderate[i];

		query_insert_moderate->bindValue(":id_message", info.IDMessage);
		query_insert_moderate->bindValue(":id_topic",   info.IDTopic);
		query_insert_moderate->bindValue(":id_user",    info.IDUser);
		query_insert_moderate->bindValue(":id_forum",   info.IDForum);
		query_insert_moderate->bindValue(":created",    info.Created.toSecsSinceEpoch());

		if (query_insert_moderate->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_insert_moderate->getLastError());
		}
	}

	//
	// РѕР±РЅРѕРІР»РµРЅРёРµ РїРѕРґРїРёСЃР°РЅС‹С… С„РѕСЂСѓРјРѕРІ
	//

	sql  = "";
	sql += "UPDATE\n";
	sql += "	`subscribed`\n";
	sql += "SET\n";
	sql += "	`is_first` = 0";

	QScopedPointer<AQuery> query_update_subscribed(createQuery(sql, false));

	if (query_update_subscribed.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_update_subscribed->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_update_subscribed->getLastError());
	}

	//
	// РѕС‡РёСЃС‚РєР° РѕР±РѕСЂРІР°РЅРЅС‹С… С‚РѕРїРёРєРѕРІ/СЃРѕРѕР±С‰РµРЅРёР№
	//

	sql  = "";
	sql += "DELETE FROM `broken`";

	QScopedPointer<AQuery> query_delete_broken(createQuery(sql, false));

	if (query_delete_broken.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_delete_broken->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_delete_broken->getLastError());
	}

	//
	// РІСЃС‚Р°РІРєР° СЃРїРёСЃРєР° РѕР±РѕСЂРІР°РЅРЅС‹С… РІРµС‚РѕРє
	//

	sql  = "";
	sql += "REPLACE INTO `broken`\n";
	sql += "(\n";
	sql += "	`id_message`,\n";
	sql += "	`is_topic`\n";
	sql += ")\n";
	sql += "SELECT\n";
	sql += "	`message`.`id_parent`,\n";
	sql += "	0\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "		INNER JOIN\n";
	sql += "	`message`\n";
	sql += "		ON\n";
	sql += "	`unread`.`id_message` = `message`.`id`\n";
	sql += "WHERE\n";
	sql += "	`message`.`id_parent` <> 0 AND\n";
	sql += "	`message`.`id_parent` NOT IN\n";
	sql += "	(\n";
	sql += "		SELECT\n";
	sql += "			`id`\n";
	sql += "		FROM\n";
	sql += "			`message`\n";
	sql += "	)\n";

	/*
	// РїРѕРїС‹С‚РєР° СЃСЂР°Р·Сѓ СЃРіСЂСѓРїРїРёСЂРѕРІР°С‚СЊ Р·Р°РїРёСЃРё РїСЂРёРІРѕРґРёС‚ Рє РґРёРєРѕРјСѓ РїР»Р°РЅСѓ РІС‹РїРѕР»РЅРµРЅРёСЏ,
	// РІ СЂРµР·СѓР»СЊС‚Р°С‚Рµ С‡РµРіРѕ Р·Р°РїСЂРѕСЃ Р±СѓРґРµС‚ РІС‹РїРѕР»РЅСЏС‚СЊСЃСЏ РѕС‡РµРЅСЊ РґРѕР»РіРѕ РїРѕ СЃСЂР°РІРЅРµРЅРёСЋ СЃ Р·Р°РїСЂРѕСЃРѕРј,
	// РіРґРµ РµСЃС‚СЊ РёР·Р±С‹С‚РѕС‡РЅС‹Рµ Р·Р°РјРµС‰РµРЅРёСЏ СѓР¶Рµ РІСЃС‚Р°РІР»РµРЅРЅС‹С… Р·Р°РїРёСЃРµР№
	sql += "GROUP BY\n";
	sql += "	`message`.`id_parent`";
	*/

	QScopedPointer<AQuery> query_insert_broken_message(createQuery(sql, false));

	if (query_insert_broken_message.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_insert_broken_message->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_insert_broken_message->getLastError());
	}

	//
	// РІСЃС‚Р°РІРєР° РѕР±РѕСЂРІР°РЅРЅС‹С… С‚РѕРїРёРєРѕРІ
	//

	sql  = "";
	sql += "REPLACE INTO `broken`\n";
	sql += "(\n";
	sql += "	`id_message`,\n";
	sql += "	`is_topic`\n";
	sql += ")\n";
	sql += "SELECT\n";
	sql += "	`message`.`id_topic`,\n";
	sql += "	1\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "		INNER JOIN\n";
	sql += "	`message`\n";
	sql += "		ON\n";
	sql += "	`unread`.`id_message` = `message`.`id`\n";
	sql += "WHERE\n";
	sql += "	`message`.`id_topic` <> 0 AND\n";
	sql += "	`message`.`id_topic` NOT IN\n";
	sql += "	(\n";
	sql += "		SELECT\n";
	sql += "			`id`\n";
	sql += "		FROM\n";
	sql += "			`message`\n";
	sql += "	)\n";

	/*
	// РїРѕРїС‹С‚РєР° СЃСЂР°Р·Сѓ СЃРіСЂСѓРїРїРёСЂРѕРІР°С‚СЊ Р·Р°РїРёСЃРё РїСЂРёРІРѕРґРёС‚ Рє РґРёРєРѕРјСѓ РїР»Р°РЅСѓ РІС‹РїРѕР»РЅРµРЅРёСЏ,
	// РІ СЂРµР·СѓР»СЊС‚Р°С‚Рµ С‡РµРіРѕ Р·Р°РїСЂРѕСЃ Р±СѓРґРµС‚ РІС‹РїРѕР»РЅСЏС‚СЊСЃСЏ РѕС‡РµРЅСЊ РґРѕР»РіРѕ РїРѕ СЃСЂР°РІРЅРµРЅРёСЋ СЃ Р·Р°РїСЂРѕСЃРѕРј,
	// РіРґРµ РµСЃС‚СЊ РёР·Р±С‹С‚РѕС‡РЅС‹Рµ Р·Р°РјРµС‰РµРЅРёСЏ СѓР¶Рµ РІСЃС‚Р°РІР»РµРЅРЅС‹С… Р·Р°РїРёСЃРµР№
	sql += "GROUP BY\n";
	sql += "	`message`.`id_topic`";
	*/

	QScopedPointer<AQuery> query_insert_broken_topic(createQuery(sql, false));

	if (query_insert_broken_topic.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_insert_broken_topic->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_insert_broken_topic->getLastError());
	}

	//
	// Р·Р°РІРµСЂС€РµРЅРёРµ С‚СЂР°РЅР·Р°РєС†РёРё
	//

	if (ASQLiteDatabase::commit() == false)
		return returnError(ASQLiteDatabase::getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getUnreadCount (AUnreadForumCountInfoList& list, int id_me)
{
	QString sql;

	// Р·Р°РїСЂРѕСЃ РґР»СЏ РІС‹Р±РѕСЂРєРё id С„РѕСЂСѓРјРѕРІ СЃ РЅРµРїСЂРѕС‡РёС‚Р°РЅРЅС‹РјРё СЃРѕРѕР±С‰РµРЅРёСЏРјРё
	// Рё РѕР±С‰РёРј РєРѕР»РёС‡РµСЃС‚РІРѕРј РЅРµРїСЂРѕС‡РёС‚Р°РЅРЅС‹С… СЃРѕРѕР±С‰РµРЅРёР№ РґР»СЏ РєР°Р¶РґРѕРіРѕ С„РѕСЂСѓРјР°
	sql += "SELECT\n";
	sql += "	`id_forum`,\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "GROUP BY\n";
	sql += "	`id_forum`";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	// Р·Р°РїСЂРѕСЃ РґР»СЏ РІС‹Р±РѕСЂРєРё РєРѕР»РёС‡РµСЃС‚РІР° СЃРѕРѕР±С‰РµРЅРёР№ РјРЅРµ РІ С„РѕСЂСѓРјРµ
	sql =  "";
	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_forum`       = :id_forum AND\n";
	sql += "	`id_parent_user` = " + QString::number(id_me);

	QScopedPointer<AQuery> query_select_my(createQuery(sql));

	if (query_select_my.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	// Р·Р°РїСЂРѕСЃ РґР»СЏ РІС‹Р±РѕСЂРєРё РєРѕР»РёС‡РµСЃС‚РІР° РЅРѕРІС‹С… С‚РѕРїРёРєРѕРІ РІ С„РѕСЂСѓРјРµ
	sql =  "";
	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_forum` = :id_forum AND\n";
	sql += "	`id_topic` = 0";

	QScopedPointer<AQuery> query_select_topics(createQuery(sql));

	if (query_select_topics.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		AUnreadForumCountInfo info;

		info.IDForum = query_select->value(0).toInt();
		info.Count   = query_select->value(1).toInt();

		query_select_my->bindValue(":id_forum", info.IDForum);
		query_select_topics->bindValue(":id_forum", info.IDForum);

		if (query_select_my->exec() == false)
			return returnError(query_select_my->getLastError());

		if (query_select_topics->exec() == false)
			return returnError(query_select_topics->getLastError());

		query_select_my->next();
		query_select_topics->next();

		info.CountMy = query_select_my->value(0).toInt();
		info.Topics  = query_select_topics->value(0).toInt();

		list.append(info);
	}

	//
	// РїРѕР»СѓС‡РµРЅРёРµ РєРѕР»РёС‡РµСЃС‚РІР° РёР· СЃРїРµС†-РїР°РїРѕРє
	//

	// СЃРѕРѕР±С‰РµРЅРёСЏ Рє РѕС‚РїСЂР°РІРєРµ
	sql  = "";
	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`message2send`\n";
	sql += "WHERE\n";
	sql += "	`draft` = 0\n";

	QScopedPointer<AQuery> query_message(createQuery(sql, false));

	if (query_message.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_message->exec() == false)
		return returnError(query_message->getLastError());

	query_message->next();

	AUnreadForumCountInfo info_message;

	info_message.IDForum = SPECIAL_ID_FORUM_MESSAGE2SEND;
	info_message.Count   = query_message->value(0).toInt();
	info_message.CountMy = 0;

	if (info_message.Count > 0)
		list.append(info_message);

	// СЂРµР№С‚РёРЅРі Рє РѕС‚РїСЂР°РІРєРµ
	sql  = "";
	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`rating2send`";

	QScopedPointer<AQuery> query_rating(createQuery(sql, false));

	if (query_rating.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_rating->exec() == false)
		return returnError(query_rating->getLastError());

	query_rating->next();

	AUnreadForumCountInfo info_rating;

	info_rating.IDForum = SPECIAL_ID_FORUM_RATING2SEND;
	info_rating.Count   = query_rating->value(0).toInt();
	info_rating.CountMy = 0;

	if (info_rating.Count > 0)
		list.append(info_rating);

	// РјРѕРґРµСЂРёР»РєРё Рє РѕС‚РїСЂР°РІРєРµ
	sql  = "";
	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`moderate2send`";

	QScopedPointer<AQuery> query_moderate(createQuery(sql, false));

	if (query_moderate.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_moderate->exec() == false)
		return returnError(query_moderate->getLastError());

	query_moderate->next();

	AUnreadForumCountInfo info_moderate;

	info_moderate.IDForum = SPECIAL_ID_FORUM_MODERATE2SEND;
	info_moderate.Count   = query_moderate->value(0).toInt();
	info_moderate.CountMy = 0;

	if (info_moderate.Count > 0)
		list.append(info_moderate);

	// С‡РµСЂРЅРѕРІРёРєРё
	sql  = "";
	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`message2send`\n";
	sql += "WHERE\n";
	sql += "	`draft` > 0\n";

	QScopedPointer<AQuery> query_draft(createQuery(sql, false));

	if (query_draft.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_draft->exec() == false)
		return returnError(query_draft->getLastError());

	query_draft->next();

	AUnreadForumCountInfo info_draft;

	info_draft.IDForum = SPECIAL_ID_FORUM_DRAFTS;
	info_draft.Count   = query_draft->value(0).toInt();
	info_draft.CountMy = 0;

	if (info_draft.Count > 0)
		list.append(info_draft);

	// РјРѕРё СЃРѕРѕР±С‰РµРЅРёСЏ
	// РїРѕ С…РѕСЂРѕС€РµРјСѓ, РЅР°РґРѕ РїСЂРѕСЃС‚Рѕ РІ С‚Р°Р±Р»РёС‡РєСѓ unread РґРѕР±Р°РІРёС‚СЊ РїРѕР»Рµ id_user Рё РёРЅРґРµРєСЃ РґР»СЏ РїРѕР»СЊР·РѕРІР°С‚РµР»СЏ,
	// С‡С‚Рѕ СЃРёР»СЊРЅРѕ СѓСЃРєРѕСЂРёС‚ Р·Р°РїСЂРѕСЃ, РѕРґРЅР°РєРѕ, РјРµРЅСЏС‚СЊ СЃС…РµРјСѓ РїРѕРєР° РЅРµ С…РѕС‡РµС‚СЃСЏ
	sql  = "";
	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "		INNER JOIN\n";
	sql += "	`message`\n";
	sql += "		ON\n";
	sql += "	`unread`.`id_message` = `message`.`id` AND\n";
	sql += "	`message`.`id_user`   = " + QString::number(id_me);

	QScopedPointer<AQuery> query_my_messages(createQuery(sql, false));

	if (query_my_messages.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_my_messages->exec() == false)
		return returnError(query_my_messages->getLastError());

	query_my_messages->next();

	AUnreadForumCountInfo info_my_messages;

	info_my_messages.IDForum = SPECIAL_ID_FORUM_MY_MESSAGES;
	info_my_messages.Count   = query_my_messages->value(0).toInt();
	info_my_messages.CountMy = 0;

	if (info_my_messages.Count > 0)
		list.append(info_my_messages);

	// РѕС‚РІРµС‚С‹ РјРЅРµ
	sql  = "";
	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_parent_user` = " + QString::number(id_me);

	QScopedPointer<AQuery> query_answers_to_me(createQuery(sql, false));

	if (query_answers_to_me.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_answers_to_me->exec() == false)
		return returnError(query_answers_to_me->getLastError());

	query_answers_to_me->next();

	AUnreadForumCountInfo info_answers_to_me;

	info_answers_to_me.IDForum = SPECIAL_ID_FORUM_ANSWERS_TO_ME;
	info_answers_to_me.Count   = query_answers_to_me->value(0).toInt();
	info_answers_to_me.CountMy = query_answers_to_me->value(0).toInt();

	if (info_answers_to_me.Count > 0)
		list.append(info_answers_to_me);

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getForumTopicList (int id_forum, int count, QList<int>& list)
{
	QString sql;

	//
	// РїРѕР»СѓС‡РµРЅРёРµ С‚РѕРїРёРєРѕРІ РґР»СЏ С„РѕСЂСѓРјР°
	//

	sql += "SELECT\n";
	sql += "	`id_message`\n";
	sql += "FROM\n";
	sql += "	`message_topic`\n";
	sql += "WHERE\n";
	sql += "	`id_forum` = " + QString::number(id_forum) + "\n";
	sql += "ORDER BY\n";
	// РїРѕСЃРєР»СЊРєСѓ id_message Р°РІС‚РѕРёРЅРєСЂРµРјРµРЅС‚ Рё СЂР°СЃС‚РµС‚ РІРјРµСЃС‚Рµ СЃ РґР°С‚РѕР№, С‚Рѕ
	// РјРѕР¶РЅРѕ СѓСЃР»РѕРІРЅРѕ РїРѕР»РѕР¶РёС‚СЊСЃСЏ РЅР° С‚Рѕ, С‡С‚Рѕ Р±РћР»СЊС€РµРјСѓ id_message СЃРѕРѕС‚РІРµС‚СЃС‚РІСѓРµС‚
	// Р±РћР»СЊС€Р°СЏ РґР°С‚Р° - РІ СЂРµР·СѓР»СЊС‚Р°С‚Рµ СЃРѕСЂС‚РёСЂРѕРІРєР° РґР°РµС‚ РІС‹РёРіСЂС‹С€ РІ РїСЂРѕРёР·РІРѕРґРёС‚РµР»СЊРЅРѕСЃС‚Рё Р±РѕР»РµРµ С‡РµРј РІ 10 СЂР°Р·
	//sql += "	`message_date` DESC";
	sql += "	`id_message` DESC";

	if (count > 0)
	{
		sql += "\nLIMIT\n";
		sql += "	" + QString::number(count);
	}

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
		list.append(query_select->value(0).toInt());

	//
	// РїРѕР»СѓС‡РµРЅРёРµ СЃРїРёСЃРєР° С‚РѕРїРёРєРѕРІ РІ РєРѕС‚РѕСЂС‹С… РµСЃС‚СЊ РЅРµРїСЂРѕС‡РёС‚Р°РЅС‹Рµ СЃРѕРѕР±С‰РµРЅРёСЏ
	//

	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_topic`\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_forum`  = " + QString::number(id_forum) + " AND\n";
	sql += "	`id_topic` <> 0\n";
	sql += "GROUP BY\n";
	sql += "	`id_topic`\n";
	sql += "ORDER BY\n";
	sql += "	`message_date`";

	QScopedPointer<AQuery> query_select_unread_child(createQuery(sql, false));

	if (query_select_unread_child.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select_unread_child->setForwardOnly(true);

	if (query_select_unread_child->exec() == false)
		return returnError(query_select_unread_child->getLastError());

	while (query_select_unread_child->next())
	{
		int id = query_select_unread_child->value(0).toInt();

		// РµСЃР»Рё С‡РёСЃР»Рѕ С‚РѕРїРёРєРѕРІ РѕРіСЂР°РЅРёС‡РµРЅРѕ Р»РёРјРёС‚РѕРј, С‚Рѕ РІРѕР·РјРѕР¶РЅР° СЃРёС‚СѓР°С†РёСЏ,
		// РєРѕРіРґР° РЅРµРїСЂРѕС‡РёС‚Р°РЅРЅС‹Рµ С‚РѕРїРёРєРё РЅРµ Р±СѓРґСѓС‚ РѕС‚РѕР±СЂР°Р¶Р°С‚СЊСЃСЏ (СЃРј. #49)
		bool found = false;

		for (int i = 0; i < list.count(); i++)
			if (list[i] == id)
			{
				found = true;

				list.move(i, 0);

				break;
			}

		if (found == false)
			list.prepend(id);
	}

	//
	// РїРѕР»СѓС‡РµРЅРёРµ СЃРїРёСЃРєР° РЅРµРїСЂРѕС‡РёС‚Р°РЅС‹С… С‚РѕРїРёРєРѕРІ
	//

	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_message`\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_forum` = " + QString::number(id_forum) + " AND\n";
	sql += "	`id_topic` = 0\n";
	sql += "ORDER BY\n";
	sql += "	`message_date`";

	QScopedPointer<AQuery> query_select_unread_topic(createQuery(sql, false));

	if (query_select_unread_topic.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select_unread_topic->setForwardOnly(true);

	if (query_select_unread_topic->exec() == false)
		return returnError(query_select_unread_topic->getLastError());

	while (query_select_unread_topic->next())
	{
		int id = query_select_unread_topic->value(0).toInt();

		bool found = false;

		for (int i = 0; i < list.count(); i++)
			if (list[i] == id)
			{
				found = true;

				list.move(i, 0);

				break;
			}

		if (found == false)
			list.prepend(id);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getTopicInfoList (int id_forum, AMessageInfoGUIPtrList& list, int id_me)
{
	// Р·Р°РїРѕР»РЅРµРЅРёРµ id-С€РЅРёРєРѕРІ СЃРѕРѕР±С‰РµРЅРёР№ (С‚РѕРїРёРєРѕРІ), РєРѕС‚РѕСЂС‹Рµ С‚СЂРµР±СѓРµС‚СЃСЏ РїРѕР»СѓС‡РёС‚СЊ
	QString ids;

	for (int i = 0; i < list.count(); i++)
	{
		ids += QString::number(list[i]->ID);

		if (i < list.count() - 1)
			ids += ", ";
	}

	QString sql;

	//
	// РїРѕР»СѓС‡РµРЅРёРµ РёРЅС„С‹ С‚РѕРїРёРєР°
	// С‡Р°СЃС‚СЊ РїРѕР»РµР№, РЅРµ РїСЂРёРЅРёРјР°СЋС‰РёС… СѓС‡Р°СЃС‚РёСЏ РІ GUI, РїСЂРѕРїСѓС‰РµРЅР°
	//

	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`id_user`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`subject`,\n";
	sql += "	`user_nick`,\n";
	sql += "	`message_date`,\n";
	sql += "	`has_child`\n";

	// РґР»СЏ РїРѕРјРµС‚РєРё РІРµС‚РѕРє РєР°Рє РїСЂРѕС‡РёС‚Р°РЅРЅС‹С… РІ РґРµСЂРµРІРµ СЃРїРµС†-С„РѕСЂСѓРјРѕРІ "РњРѕРё СЃРѕРѕР±С‰РµРЅРёСЏ" Рё "РћС‚РІРµС‚С‹ РјРЅРµ"
	if (id_forum == SPECIAL_ID_FORUM_MY_MESSAGES || id_forum == SPECIAL_ID_FORUM_ANSWERS_TO_ME)
		sql += ",\n	id_topic";

	if (id_forum == SPECIAL_ID_FORUM_MY_MESSAGES)
		sql += ",\n	id_parent\n";
	else
		sql += "\n";

	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id` IN (" + ids + ")";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		int id = query_select->value(0).toInt();

		for (int i = 0; i < list.count(); i++)
		{
			AMessageInfoGUI* info = list[i];

			if (id == info->ID)
			{
				// С‡Р°СЃС‚СЊ РїРѕР»РµР№, РЅРµ РїСЂРёРЅРёРјР°СЋС‰РёС… СѓС‡Р°СЃС‚РёСЏ РІ GUI, РѕСЃС‚Р°РІР»РµРЅР° Р±РµР· РёР·РјРµРЅРµРЅРёР№
				info->IDTopic  = 0; // РІСЃРµРіРґР° 0 РґР»СЏ С‚РѕРїРёРєРѕРІ
				info->IDParent = 0; // РІСЃРµРіРґР° 0 РґР»СЏ С‚РѕРїРёРєРѕРІ
				info->IDUser   = query_select->value(1).toInt();

				if (id_forum >= 0)
					info->IDForum = id_forum;
				else
					info->IDForum = query_select->value(2).toInt();

				info->Subject     = query_select->value(3).toString();
				info->UserNick    = query_select->value(4).toString();
				info->MessageDate = QDateTime::fromSecsSinceEpoch(query_select->value(5).toUInt());

				if (id_forum >= 0)
					info->HasChild = query_select->value(6).toInt();
				else if (id_forum == SPECIAL_ID_FORUM_MY_MESSAGES)
				{
					info->IDTopic  = query_select->value(7).toInt();
					info->IDParent = query_select->value(8).toInt();
				}
				else if (id_forum == SPECIAL_ID_FORUM_ANSWERS_TO_ME)
					info->IDTopic  = query_select->value(7).toInt();

				break;
			}
		}
	}

	//
	// РїРѕР»СѓС‡РµРЅРёРµ С„Р»Р°РіР° РѕС‚РІРµС‚Р° РјРЅРµ (С‚РѕР»СЊРєРѕ РґР»СЏ СЃРїРµС†-С„РѕСЂСѓРјР° "РњРѕРё СЃРѕРѕР±С‰РµРЅРёСЏ")
	//

	if (id_forum == SPECIAL_ID_FORUM_MY_MESSAGES)
	{
		sql  = "";
		sql += "SELECT\n";
		sql += "	`m1`.`id`\n";
		sql += "FROM\n";
		sql += "	`message` `m1`\n";
		sql += "		INNER JOIN\n";
		sql += "	`message` `m2`\n";
		sql += "		ON\n";
		sql += "	`m1`.`id` IN (" + ids + ") AND\n";
		sql += "	`m1`.`id_parent` <> 0 AND\n";
		sql += "	`m1`.`id_parent`  = `m2`.`id` AND\n";
		sql += "	`m2`.`id_user`    = " + QString::number(id_me);

		QScopedPointer<AQuery> query_select_my_me(createQuery(sql, false));

		if (query_select_my_me.isNull() == true)
			return returnError(ASQLiteDatabase::getLastError());

		query_select_my_me->setForwardOnly(true);

		if (query_select_my_me->exec() == false)
			return returnError(query_select_my_me->getLastError());

		while (query_select_my_me->next())
		{
			int id = query_select_my_me->value(0).toInt();

			for (int i = 0; i < list.count(); i++)
			{
				AMessageInfoGUI* info = list[i];

				if (id == info->ID)
				{
					info->Special = id_me;
					break;
				}
			}
		}
	}

	//
	// РїРѕР»СѓС‡РµРЅРёРµ СЃРїРёСЃРєР° РЅРµРїСЂРѕС‡РёС‚Р°РЅС‹С… С‚РѕРїРёРєРѕРІ Рё РёС… РѕС‚РјРµС‚РєР° РІ Р·Р°РїСЂРѕС€РµРЅРЅРѕРј СЃРїРёСЃРєРµ
	//

	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_message`\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_message` IN (" + ids + ")";

	QScopedPointer<AQuery> query_select_unread(createQuery(sql, false));

	if (query_select_unread.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select_unread->setForwardOnly(true);

	if (query_select_unread->exec() == false)
		return returnError(query_select_unread->getLastError());

	while (query_select_unread->next())
	{
		int id = query_select_unread->value(0).toInt();

		for (int i = 0; i < list.count(); i++)
		{
			AMessageInfoGUI* info = list[i];

			if (id == info->ID)
			{
				info->IsRead = false;
				break;
			}
		}
	}

	// РґР»СЏ РѕС‚РІРµС‚РѕРІ РјРЅРµ Рё РјРѕРёС… СЃРѕРѕР±С‰РµРЅРёР№ РґРѕС‡РµСЂРЅРёРµ РЅРµ РїРѕРєР°Р·С‹РІР°СЋС‚СЃСЏ
	if (id_forum < 0)
		return returnSuccess();

	//
	// РїРѕР»СѓС‡РµРЅРёРµ СЃРїРёСЃРєР° РЅРµРїСЂРѕС‡РёС‚Р°РЅС‹С… РґРѕС‡РµСЂРЅРёС… Рё РёС… РѕС‚РјРµС‚РєР° РІ Р·Р°РїСЂРѕС€РµРЅРЅРѕРј СЃРїРёСЃРєРµ
	//

	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_topic`\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_topic` IN (" + ids + ")\n";
	sql += "GROUP BY\n";
	sql += "	`id_topic`\n";

	QScopedPointer<AQuery> query_select_unread_child(createQuery(sql, false));

	if (query_select_unread_child.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select_unread_child->setForwardOnly(true);

	if (query_select_unread_child->exec() == false)
		return returnError(query_select_unread_child->getLastError());

	while (query_select_unread_child->next())
	{
		int id = query_select_unread_child->value(0).toInt();

		for (int i = 0; i < list.count(); i++)
		{
			AMessageInfoGUI* info = list[i];

			if (id == info->ID)
			{
				info->HasUnreadChild = true;

				break;
			}
		}
	}

	//
	// РїРѕР»СѓС‡РµРЅРёРµ СЃРїРёСЃРєР° РЅРµРїСЂРѕС‡РёС‚Р°РЅС‹С… РґРѕС‡РµСЂРЅРёС… РѕС‚РІРµС‚РѕРІ РјРЅРµ Рё РёС… РѕС‚РјРµС‚РєР° РІ Р·Р°РїСЂРѕС€РµРЅРЅРѕРј СЃРїРёСЃРєРµ
	//

	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_topic`\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_parent_user` = " + QString::number(id_me) + " AND\n";
	sql += "	`id_topic` IN (" + ids + ")\n";
	sql += "GROUP BY\n";
	sql += "	`id_topic`\n";

	QScopedPointer<AQuery> query_select_unread_child_my(createQuery(sql, false));

	if (query_select_unread_child_my.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select_unread_child_my->setForwardOnly(true);

	if (query_select_unread_child_my->exec() == false)
		return returnError(query_select_unread_child_my->getLastError());

	while (query_select_unread_child_my->next())
	{
		int id = query_select_unread_child_my->value(0).toInt();

		for (int i = 0; i < list.count(); i++)
		{
			AMessageInfoGUI* info = list[i];

			if (id == info->ID)
			{
				info->HasUnreadChildMy = true;

				break;
			}
		}
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getTopicMessageList (int id_forum, int id_topic, AMessageInfoGUIPtrList& list, IMessageInfoGUIFactory* factory)
{
	QString sql;

	//
	// РїРѕР»СѓС‡РµРЅРёРµ СЃРѕРѕР±С‰РµРЅРёР№ С‚РѕРїРёРєР°
	// С‡Р°СЃС‚СЊ РїРѕР»РµР№, РЅРµ РїСЂРёРЅРёРјР°СЋС‰РёС… СѓС‡Р°СЃС‚РёСЏ РІ GUI, РїСЂРѕРїСѓС‰РµРЅР°
	//

	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`id_parent`,\n";
	sql += "	`id_user`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`subject`,\n";
	sql += "	`user_nick`,\n";
	sql += "	`message_date`,\n";
	sql += "	`has_child`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id_topic` = " + QString::number(id_topic) + "\n";
	sql += "ORDER BY\n";
	// РїРѕСЃРєР»СЊРєСѓ id Р°РІС‚РѕРёРЅРєСЂРµРјРµРЅС‚ Рё СЂР°СЃС‚РµС‚ РІРјРµСЃС‚Рµ СЃ РґР°С‚РѕР№, С‚Рѕ
	// РјРѕР¶РЅРѕ СѓСЃР»РѕРІРЅРѕ РїРѕР»РѕР¶РёС‚СЊСЃСЏ РЅР° С‚Рѕ, С‡С‚Рѕ Р±РћР»СЊС€РµРјСѓ id_message СЃРѕРѕС‚РІРµС‚СЃС‚РІСѓРµС‚
	// Р±РћР»СЊС€Р°СЏ РґР°С‚Р° - РІ СЂРµР·СѓР»СЊС‚Р°С‚Рµ СЃРѕСЂС‚РёСЂРѕРІРєР° РґР°РµС‚ РІС‹РёРіСЂС‹С€ РІ РїСЂРѕРёР·РІРѕРґРёС‚РµР»СЊРЅРѕСЃС‚Рё РІ Р±РѕР»РµРµ С‡РµРј 10 СЂР°Р·
	//sql += "	`message_date` DESC";
	sql += "	`id` DESC";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		AMessageInfoGUI* info = factory->createItem();

		// С‡Р°СЃС‚СЊ РїРѕР»РµР№, РЅРµ РїСЂРёРЅРёРјР°СЋС‰РёС… СѓС‡Р°СЃС‚РёСЏ РІ GUI, РѕСЃС‚Р°РІР»РµРЅР° Р±РµР· РёР·РјРµРЅРµРЅРёР№
		info->ID       = query_select->value(0).toInt();
		info->IDTopic  = id_topic;
		info->IDParent = query_select->value(1).toInt();
		info->IDUser   = query_select->value(2).toInt();

		if (id_forum != 0)
			info->IDForum = id_forum;
		else
			info->IDForum = query_select->value(3).toInt();

		info->Subject     = query_select->value(4).toString();
		info->UserNick    = query_select->value(5).toString();
		info->MessageDate = QDateTime::fromSecsSinceEpoch(query_select->value(6).toUInt());
		info->HasChild    = query_select->value(7).toInt();

		list.append(info);
	}

	//
	// РЅРµРїСЂРѕС‡РёС‚Р°РЅРЅС‹Рµ СЃРѕРѕР±С‰РµРЅРёСЏ
	//

	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_message`\n";
	sql += "FROM\n";
	sql += "	`unread`\n";
	sql += "WHERE\n";
	sql += "	`id_topic` = " + QString::number(id_topic) + "\n";
	sql += "ORDER BY\n";
	sql += "	`message_date`";

	QScopedPointer<AQuery> query_select_unread(createQuery(sql, false));

	if (query_select_unread.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select_unread->setForwardOnly(true);

	if (query_select_unread->exec() == false)
		return returnError(query_select_unread->getLastError());

	while (query_select_unread->next())
	{
		int id = query_select_unread->value(0).toInt();

		for (int i = 0; i < list.count(); i++)
			if (list[i]->ID == id)
			{
				list[i]->IsRead = false;

				list.move(i, 0);

				break;
			}
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMessageBody (int id_message, QString& body)
{
	QString sql;

	//
	// РїРѕР»СѓС‡РµРЅРёРµ С‚РµР»Р° СЃРѕРѕР±С‰РµРЅРёСЏ
	//

	sql += "SELECT\n";
	sql += "	`message`,\n";
	sql += "	`compressed`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id` = " + QString::number(id_message);

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		// С„Р»Р°Рі СЃР¶Р°С‚РёСЏ СЃРѕРѕР±С‰РµРЅРёСЏ
		bool compressed = query_select->value(1).toInt();

		if (compressed == false)
			body = query_select->value(0).toString();
		else
		{
			#ifndef AVALON_USE_ZLIB

			body = QString::fromUtf8("(С‚РµР»Рѕ СЃРѕРѕР±С‰РµРЅРёСЏ СЃР¶Р°С‚Рѕ, СЃРєРѕРјРїРёР»РёСЂСѓР№С‚Рµ avalon СЃ РїРѕРґРґРµСЂР¶РєРѕР№ zlib)");

			#else

			QByteArray message = query_select->value(0).toByteArray();

			uLongf       compressed_buf_size = message.count();
			const Bytef* compressed_buf      = (const Bytef*)message.constData();

			// РїРѕ С‚РµСЃС‚Р°Рј СЃР¶Р°С‚РёРµ СЃРѕСЃС‚Р°РІР»СЏРµС‚ РІ СЃСЂРµРґРЅРµРј ~50%
			uLongf       uncompressed_buf_size      = compressed_buf_size * 3;
			Bytef*       uncompressed_buf           = new Bytef[uncompressed_buf_size];
			uLongf       uncompressed_buf_size_real = uncompressed_buf_size;

			while (true)
			{
				int z_result = uncompress(uncompressed_buf, &uncompressed_buf_size_real, compressed_buf, compressed_buf_size);

				if (z_result == Z_OK)
				{
					body = QString::fromUtf8(reinterpret_cast<const char*>(uncompressed_buf), uncompressed_buf_size_real);

					delete [] uncompressed_buf;

					break;
				}
				else if (z_result == Z_BUF_ERROR)
				{
					// РЅРµ С…РІР°С‚РёР»Рѕ РґР»РёРЅС‹ Р±СѓС„РµСЂР° РґР»СЏ СЂР°СЃРїР°РєРѕРІРєРё
					delete [] uncompressed_buf;

					uncompressed_buf_size     *= 2;
					uncompressed_buf           = new Bytef[uncompressed_buf_size];
					uncompressed_buf_size_real = uncompressed_buf_size;

					continue;
				}

				delete [] uncompressed_buf;

				// РїСЂРѕС‡РёРµ С„Р°С‚Р°Р»СЊРЅС‹Рµ РѕС€РёР±РєРё
				if (z_result == Z_MEM_ERROR)
					return returnError("Z_MEM_ERROR: there was not enough memory.");
				else if (z_result == Z_DATA_ERROR)
					return returnError("Z_DATA_ERROR: the input data was corrupted.");
				else
					return returnError("Z_UNKNOWN_ERROR: z_result = " + QString::number(z_result));

			}   // while (true)

			#endif   // #ifdef AVALON_USE_ZLIB

		}   // if (compressed == false) ... else

	}   // while (query_select->next())

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::setIDsAsRead (const QList<int>& list, AIDSet type, bool read, QDateTime date)
{
	// РїРѕР»СѓС‡РµРЅРёРµ СЃС‚СЂРѕРєРё СЃ id
	QString ids;

	for (int i = 0; i < list.count(); i++)
	{
		ids += QString::number(list[i]);

		if (i < list.count() - 1)
			ids += ", ";
	}

	QString sql;
	QString sql_2;

	//
	// РїРѕР»СѓС‡РµРЅРёРµ С‚РµР»Р° СЃРѕРѕР±С‰РµРЅРёСЏ
	//

	if (read == true)
	{
		switch (type)
		{
			case idsMessage:

				sql += "DELETE FROM\n";
				sql += "	`unread`\n";
				sql += "WHERE\n";
				sql += "	`id_message` IN (" + ids + ")";

				if (date.isValid() == true)
					sql += " AND\n	`message_date` <= " + QString::number(date.toSecsSinceEpoch());

				break;

			case idsTopic:

				// Р·Р°РїСЂРѕСЃ РјРѕР¶РµС‚ РґР°РІР°С‚СЊ РѕС‡РµРЅСЊ РїР»РѕС…РѕР№ РїР»Р°РЅ РІС‹РїРѕР»РЅРµРЅРёСЏ (СЃРј. Р°РЅР°Р»РѕРіРёС‡РЅС‹Р№ РїСЂРё РїРѕРјРµС‚РєРµ РєР°Рє РЅРµРїСЂРѕС‡РёС‚Р°РЅРЅРѕРµ),
				// РѕРґРЅР°РєРѕ, РІ `unread` РѕР±С‹С‡РЅРѕ СЃР»РёС€РєРѕРј РјР°Р»Рѕ СЃС‚СЂРѕРє, С‡С‚РѕР±С‹ СЌС‚Рѕ Р±С‹Р»Рѕ Р·Р°РјРµС‚РЅРѕ
				sql += "DELETE FROM\n";
				sql += "	`unread`\n";
				sql += "WHERE\n";
				sql += "	(\n";
				sql += "		`id_message` IN (" + ids + ") OR\n";
				sql += "		`id_topic`   IN (" + ids + ")\n";
				sql += "	)";

				if (date.isValid() == true)
					sql += " AND\n	`message_date` <= " + QString::number(date.toSecsSinceEpoch());

				break;

			case idsForum:

				if (list.count() == 1 && (list[0] == SPECIAL_ID_FORUM_MY_MESSAGES || list[0] == SPECIAL_ID_FORUM_ANSWERS_TO_ME))
				{
					// РїРѕР»СѓС‡РµРЅРёРµ id С‚РµРєСѓС‰РµРіРѕ РїРѕР»СЊР·РѕРІР°С‚РµР»СЏ avalon (РїРѕ С…РѕСЂРѕС€РµРјСѓ, РґРѕР»Р¶РЅР° РїРµСЂРµРґР°РІР°С‚СЊСЃСЏ РєР°Рє РїР°СЂР°РјРµС‚СЂ)
					int id_me = AGlobal::getInstance()->Me.ID;

					if (id_me == 0)
						break;

					if (list[0] == SPECIAL_ID_FORUM_MY_MESSAGES)
					{
						// СЃРїРµС†-С„РѕСЂСѓРј "РњРѕРё СЃРѕРѕР±С‰РµРЅРёСЏ"
						// РїРѕ С…РѕСЂРѕС€РµРјСѓ, РЅР°РґРѕ РїСЂРѕСЃС‚Рѕ РІ С‚Р°Р±Р»РёС‡РєСѓ unread РґРѕР±Р°РІРёС‚СЊ РїРѕР»Рµ id_user Рё РёРЅРґРµРєСЃ РґР»СЏ РїРѕР»СЊР·РѕРІР°С‚РµР»СЏ,
						// С‡С‚Рѕ СЃРёР»СЊРЅРѕ СѓСЃРєРѕСЂРёС‚ Р·Р°РїСЂРѕСЃ, РѕРґРЅР°РєРѕ, РјРµРЅСЏС‚СЊ СЃС…РµРјСѓ РїРѕРєР° РЅРµ С…РѕС‡РµС‚СЃСЏ
						sql += "DELETE FROM\n";
						sql += "	`unread`\n";
						sql += "WHERE\n";
						sql += "	`id_message` IN\n";
						sql += "	(\n";
						sql += "		SELECT\n";
						sql += "			`unread`.`id_message`\n";
						sql += "		FROM\n";
						sql += "			`unread`\n";
						sql += "				INNER JOIN\n";
						sql += "			`message`\n";
						sql += "				ON\n";
						sql += "			`unread`.`id_message` = `message`.`id` AND\n";
						sql += "			`message`.`id_user`   = " + QString::number(id_me) + "\n";
						sql += "	)";
					}
					else if (list[0] == SPECIAL_ID_FORUM_ANSWERS_TO_ME)
					{
						// СЃРїРµС†-С„РѕСЂСѓРј "РћС‚РІРµС‚С‹ РјРЅРµ"
						sql += "DELETE FROM\n";
						sql += "	`unread`\n";
						sql += "WHERE\n";
						sql += "	`id_parent_user` = " + QString::number(id_me);
					}
				}
				else
				{
					// РІСЃРµ РѕСЃС‚Р°Р»СЊРЅС‹Рµ С„РѕСЂСѓРјС‹
					sql += "DELETE FROM\n";
					sql += "	`unread`\n";
					sql += "WHERE\n";
					sql += "	`id_forum` IN (" + ids + ")";
				}

				if (date.isValid() == true)
					sql += " AND\n	`message_date` <= " + QString::number(date.toSecsSinceEpoch());

				break;

			case idsGroup:

				sql += "DELETE FROM\n";
				sql += "	`unread`\n";
				sql += "WHERE\n";
				sql += "	`id_forum` IN\n";
				sql += "	(\n";
				sql += "		SELECT\n";
				sql += "			`id`\n";
				sql += "		FROM\n";
				sql += "			`forum`\n";
				sql += "		WHERE\n";
				sql += "			`id_group` IN (" + ids + ")\n";
				sql += "	)";

				if (date.isValid() == true)
					sql += " AND\n	`message_date` <= " + QString::number(date.toSecsSinceEpoch());

				break;

			case idsAll:

				sql += "DELETE FROM `unread`";

				if (date.isValid() == true)
					sql += "WHERE `message_date` <= " + QString::number(date.toSecsSinceEpoch());

				break;

			default:
				return returnError(QString::fromUtf8("РќРµ СѓРєР°Р·Р°РЅР° РіСЂСѓРїРїР° РѕР±СЉРµРєС‚РѕРІ"));
		}   // switch (type)
	}
	else   // if (read == true)
	{
		switch (type)
		{
			case idsMessage:

				sql += "REPLACE INTO `unread`\n";
				sql += "(\n";
				sql += "	`id_message`,\n";
				sql += "	`id_parent`,\n";
				sql += "	`id_forum`,\n";
				sql += "	`id_topic`,\n";
				sql += "	`id_parent_user`,\n";
				sql += "	`message_date`\n";
				sql += ")\n";
				sql += "SELECT\n";
				sql += "	`id`,\n";
				sql += "	`id_parent`,\n";
				sql += "	`id_forum`,\n";
				sql += "	`id_topic`,\n";
				sql += "	0,\n";
				sql += "	`message_date`\n";
				sql += "FROM\n";
				sql += "	`message`\n";
				sql += "WHERE\n";
				sql += "	`id` IN (" + ids + ")";

				if (date.isValid() == true)
					sql += " AND\n	`message_date` >= " + QString::number(date.toSecsSinceEpoch());

				break;

			case idsTopic:

				/*

				Р—Р°РїСЂРѕСЃ РІ РѕР±С‰РµРј РІРёРґРµ СЃ СѓСЃР»РѕРІРёРµРј:

				sql += "WHERE\n";
				sql += "	(\n";
				sql += "		`id`       IN (" + ids + ") OR\n";
				sql += "		`id_topic` IN (" + ids + ")";
				sql += "	)";

				РґР°РµС‚ РѕС‚РІСЂР°С‚РёС‚РµР»СЊРЅС‹Р№ РїР»Р°РЅ РІС‹РїРѕР»РЅРµРЅРёСЏ Рё РёСЃРїРѕР»РЅСЏРµС‚СЃСЏ РІ РЅРµСЃРєРѕР»СЊРєРѕ СЂР°Р· РґРѕР»СЊС€Рµ,
				РЅРµР¶РµР»Рё СЌС‚РѕС‚ Р¶Рµ Р·Р°РїСЂРѕСЃ, РЅРѕ СЂР°Р·Р±РёС‚С‹Р№ РЅР° РґРІР° РїСЂРѕСЃС‚С‹С…, С‡С‚Рѕ Рё Р±С‹Р»Рѕ СЃРґРµР»Р°РЅРѕ.

				*/

				sql += "REPLACE INTO `unread`\n";
				sql += "(\n";
				sql += "	`id_message`,\n";
				sql += "	`id_parent`,\n";
				sql += "	`id_forum`,\n";
				sql += "	`id_topic`,\n";
				sql += "	`id_parent_user`,\n";
				sql += "	`message_date`\n";
				sql += ")\n";
				sql += "SELECT\n";
				sql += "	`id`,\n";
				sql += "	`id_parent`,\n";
				sql += "	`id_forum`,\n";
				sql += "	`id_topic`,\n";
				sql += "	0,\n";
				sql += "	`message_date`\n";
				sql += "FROM\n";
				sql += "	`message`\n";
				sql += "WHERE\n";
				sql += "	`id` IN (" + ids + ")";

				if (date.isValid() == true)
					sql += " AND\n	`message_date` >= " + QString::number(date.toSecsSinceEpoch());

				sql_2 += "REPLACE INTO `unread`\n";
				sql_2 += "(\n";
				sql_2 += "	`id_message`,\n";
				sql_2 += "	`id_parent`,\n";
				sql_2 += "	`id_forum`,\n";
				sql_2 += "	`id_topic`,\n";
				sql_2 += "	`id_parent_user`,\n";
				sql_2 += "	`message_date`\n";
				sql_2 += ")\n";
				sql_2 += "SELECT\n";
				sql_2 += "	`id`,\n";
				sql_2 += "	`id_parent`,\n";
				sql_2 += "	`id_forum`,\n";
				sql_2 += "	`id_topic`,\n";
				sql_2 += "	0,\n";
				sql_2 += "	`message_date`\n";
				sql_2 += "FROM\n";
				sql_2 += "	`message`\n";
				sql_2 += "WHERE\n";
				sql_2 += "	`id_topic` IN (" + ids + ")";

				if (date.isValid() == true)
					sql_2 += " AND\n	`message_date` >= " + QString::number(date.toSecsSinceEpoch());

				break;

			case idsForum:

				if (list.count() == 1 && (list[0] == SPECIAL_ID_FORUM_MY_MESSAGES || list[0] == SPECIAL_ID_FORUM_ANSWERS_TO_ME))
				{
					// РїРѕР»СѓС‡РµРЅРёРµ id С‚РµРєСѓС‰РµРіРѕ РїРѕР»СЊР·РѕРІР°С‚РµР»СЏ avalon (РїРѕ С…РѕСЂРѕС€РµРјСѓ, РґРѕР»Р¶РЅР° РїРµСЂРµРґР°РІР°С‚СЊСЃСЏ РєР°Рє РїР°СЂР°РјРµС‚СЂ)
					int id_me = AGlobal::getInstance()->Me.ID;

					if (id_me == 0)
						break;

					if (list[0] == SPECIAL_ID_FORUM_MY_MESSAGES)
					{
						// СЃРїРµС†-С„РѕСЂСѓРј "РњРѕРё СЃРѕРѕР±С‰РµРЅРёСЏ"
						sql += "REPLACE INTO `unread`\n";
						sql += "(\n";
						sql += "	`id_message`,\n";
						sql += "	`id_parent`,\n";
						sql += "	`id_forum`,\n";
						sql += "	`id_topic`,\n";
						sql += "	`id_parent_user`,\n";
						sql += "	`message_date`\n";
						sql += ")\n";
						sql += "SELECT\n";
						sql += "	`id`,\n";
						sql += "	`id_parent`,\n";
						sql += "	`id_forum`,\n";
						sql += "	`id_topic`,\n";
						sql += "	0,\n";
						sql += "	`message_date`\n";
						sql += "FROM\n";
						sql += "	`message`\n";
						sql += "WHERE\n";
						sql += "	`id_user` = " + QString::number(id_me);
					}
					else if (list[0] == SPECIAL_ID_FORUM_ANSWERS_TO_ME)
					{
						// СЃРїРµС†-С„РѕСЂСѓРј "РћС‚РІРµС‚С‹ РјРЅРµ"
						sql += "REPLACE INTO `unread`\n";
						sql += "(\n";
						sql += "	`id_message`,\n";
						sql += "	`id_parent`,\n";
						sql += "	`id_forum`,\n";
						sql += "	`id_topic`,\n";
						sql += "	`id_parent_user`,\n";
						sql += "	`message_date`\n";
						sql += ")\n";
						sql += "SELECT\n";
						sql += "	`id`,\n";
						sql += "	`id_parent`,\n";
						sql += "	`id_forum`,\n";
						sql += "	`id_topic`,\n";
						sql += "	0,\n";
						sql += "	`message_date`\n";
						sql += "FROM\n";
						sql += "	`message`\n";
						sql += "WHERE\n";
						sql += "	`id_parent` IN\n";
						sql += "	(\n";
						sql += "		SELECT\n";
						sql += "			`id`\n";
						sql += "		FROM\n";
						sql += "			`message`\n";
						sql += "		WHERE\n";
						sql += "			`id_user` = " + QString::number(id_me) + "\n";
						sql += "	)";
					}
				}
				else
				{
					// РІСЃРµ РѕСЃС‚Р°Р»СЊРЅС‹Рµ С„РѕСЂСѓРјС‹
					sql += "REPLACE INTO `unread`\n";
					sql += "(\n";
					sql += "	`id_message`,\n";
					sql += "	`id_parent`,\n";
					sql += "	`id_forum`,\n";
					sql += "	`id_topic`,\n";
					sql += "	`id_parent_user`,\n";
					sql += "	`message_date`\n";
					sql += ")\n";
					sql += "SELECT\n";
					sql += "	`id`,\n";
					sql += "	`id_parent`,\n";
					sql += "	`id_forum`,\n";
					sql += "	`id_topic`,\n";
					sql += "	0,\n";
					sql += "	`message_date`\n";
					sql += "FROM\n";
					sql += "	`message`\n";
					sql += "WHERE\n";
					sql += "	`id_forum` IN (" + ids + ")";
				}

				if (date.isValid() == true)
					sql += " AND\n	`message_date` >= " + QString::number(date.toSecsSinceEpoch());

				break;

			case idsGroup:

				sql += "REPLACE INTO `unread`\n";
				sql += "(\n";
				sql += "	`id_message`,\n";
				sql += "	`id_parent`,\n";
				sql += "	`id_forum`,\n";
				sql += "	`id_topic`,\n";
				sql += "	`id_parent_user`,\n";
				sql += "	`message_date`\n";
				sql += ")\n";
				sql += "SELECT\n";
				sql += "	`id`,\n";
				sql += "	`id_parent`,\n";
				sql += "	`id_forum`,\n";
				sql += "	`id_topic`,\n";
				sql += "	0,\n";
				sql += "	`message_date`\n";
				sql += "FROM\n";
				sql += "	`message`\n";
				sql += "WHERE\n";
				sql += "	`id_forum` IN\n";
				sql += "	(\n";
				sql += "		SELECT\n";
				sql += "			`id`\n";
				sql += "		FROM\n";
				sql += "			`forum`\n";
				sql += "		WHERE\n";
				sql += "			`id_group` IN (" + ids + ")\n";
				sql += "	)";

				if (date.isValid() == true)
					sql += " AND\n	`message_date` >= " + QString::number(date.toSecsSinceEpoch());

				break;

			case idsAll:

				sql += "REPLACE INTO `unread`\n";
				sql += "(\n";
				sql += "	`id_message`,\n";
				sql += "	`id_parent`,\n";
				sql += "	`id_forum`,\n";
				sql += "	`id_topic`,\n";
				sql += "	`id_parent_user`,\n";
				sql += "	`message_date`\n";
				sql += ")\n";
				sql += "SELECT\n";
				sql += "	`id`,\n";
				sql += "	`id_parent`,\n";
				sql += "	`id_forum`,\n";
				sql += "	`id_topic`,\n";
				sql += "	0,\n";
				sql += "	`message_date`\n";
				sql += "FROM\n";
				sql += "	`message`";

				if (date.isValid() == true)
					sql += "\nWHERE\n	`message_date` >= " + QString::number(date.toSecsSinceEpoch());

				break;

			default:
				return returnError(QString::fromUtf8("РќРµ СѓРєР°Р·Р°РЅР° РіСЂСѓРїРїР° РѕР±СЉРµРєС‚РѕРІ"));
		}   // switch (type)
	}   // if (read == true) else ...

	QScopedPointer<AQuery> query(createQuery(sql, false));

	if (query.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query->exec() == false)
		return returnError(query->getLastError());

	if (sql_2.length() > 0)
	{
		QScopedPointer<AQuery> query_2(createQuery(sql_2, false));

		if (query_2.isNull() == true)
			return returnError(ASQLiteDatabase::getLastError());

		if (query_2->exec() == false)
			return returnError(query_2->getLastError());
	}

	// РѕР±РЅРѕРІР»РµРЅРёРµ РёРЅС„РѕСЂРјР°С†РёРё Рѕ РїРѕР»СЊР·РѕРІР°С‚РµР»Рµ, РєРѕС‚РѕСЂРѕРјСѓ Р±С‹Р» РѕС‚РІРµС‚
	if (read == false)
	{
		sql  = "";
		sql += "UPDATE\n";
		sql += "	`unread`\n";
		sql += "SET\n";
		sql += "	`id_parent_user` = IFNULL(\n";
		sql += "		(\n";
		sql += "			SELECT\n";
		sql += "				IFNULL(`message`.`id_user`, 0)\n";
		sql += "			FROM\n";
		sql += "				`message`\n";
		sql += "			WHERE\n";
		sql += "				`message`.`id` = `unread`.`id_parent`\n";
		sql += "		),\n";
		sql += " 		0\n";
		sql += "	)\n";
		sql += "WHERE\n";
		sql += "	`id_parent` <> 0";

		QScopedPointer<AQuery> query_update_unread(createQuery(sql));

		if (query_update_unread.isNull() == true)
			return returnError(ASQLiteDatabase::getLastError());

		if (query_update_unread->exec() == false)
			return returnError(query_update_unread->getLastError());
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::unsubscribe (const QList<int>& list, AIDSet type, bool clean)
{
	if (!(type == idsGroup || type == idsForum))
		return returnError(QString::fromUtf8("РЈРєР°Р·Р°РЅР° РЅРµРІРµСЂРЅР°СЏ РіСЂСѓРїРїР° РѕР±СЉРµРєС‚РѕРІ"));

	if (ASQLiteDatabase::transaction() == false)
		return returnError(ASQLiteDatabase::getLastError());

	// РїРѕР»СѓС‡РµРЅРёРµ СЃС‚СЂРѕРєРё СЃ id
	QString ids;

	for (int i = 0; i < list.count(); i++)
	{
		ids += QString::number(list[i]);

		if (i < list.count() - 1)
			ids += ", ";
	}

	QString sql;

	// РѕС‚РїРёСЃРєР° СЃ РѕС‡РёСЃС‚РєРѕР№
	if (clean == true)
	{
		//
		// СѓРґР°Р»РµРЅРёРµ РёР· СЃРїРёСЃРєР° РЅРµРїСЂРѕС‡РёС‚Р°РЅРЅС‹С…
		//

		sql += "DELETE FROM\n";
		sql += "	`unread`\n";
		sql += "WHERE\n";

		if (type == idsForum)
			sql += "	`id_forum` IN (" + ids + ")";
		else
		{
			sql += "	`id_forum` IN\n";
			sql += "	(\n";
			sql += "		SELECT\n";
			sql += "			`id`\n";
			sql += "		FROM\n";
			sql += "			`forum`\n";
			sql += "		WHERE\n";
			sql += "			`id_group` IN (" + ids + ")\n";
			sql += "	)";
		}

		QScopedPointer<AQuery> query_unread(createQuery(sql, false));

		if (query_unread.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		if (query_unread->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_unread->getLastError());
		}

		//
		// СѓРґР°Р»РµРЅРёРµ РёР· СЃРїРёСЃРєР° СЂРµР№С‚РёРЅРіР°
		//

		sql  = "";
		sql += "DELETE FROM\n";
		sql += "	`rating`\n";
		sql += "WHERE\n";
		sql += "	`id_message` IN\n";
		sql += "	(\n";
		sql += "		SELECT\n";
		sql += "			`id`\n";
		sql += "		FROM\n";
		sql += "			`message`\n";
		sql += "		WHERE\n";

		if (type == idsForum)
			sql += "			`id_forum` IN (" + ids + ")";
		else
		{
			sql += "			`id_forum` IN\n";
			sql += "			(\n";
			sql += "				SELECT\n";
			sql += "					`id`\n";
			sql += "				FROM\n";
			sql += "					`forum`\n";
			sql += "				WHERE\n";
			sql += "					`id_group` IN (" + ids + ")\n";
			sql += "			)";
		}

		sql += "	)";

		QScopedPointer<AQuery> query_rating(createQuery(sql, false));

		if (query_rating.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		if (query_rating->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_rating->getLastError());
		}

		//
		// СѓРґР°Р»РµРЅРёРµ РёР· СЃРїРёСЃРєР° РјРѕРґРµСЂРёР»РѕРє
		//

		sql  = "";
		sql += "DELETE FROM\n";
		sql += "	`moderate`\n";
		sql += "WHERE\n";
		sql += "	`id_message` IN\n";
		sql += "	(\n";
		sql += "		SELECT\n";
		sql += "			`id`\n";
		sql += "		FROM\n";
		sql += "			`message`\n";
		sql += "		WHERE\n";

		if (type == idsForum)
			sql += "			`id_forum` IN (" + ids + ")";
		else
		{
			sql += "			`id_forum` IN\n";
			sql += "			(\n";
			sql += "				SELECT\n";
			sql += "					`id`\n";
			sql += "				FROM\n";
			sql += "					`forum`\n";
			sql += "				WHERE\n";
			sql += "					`id_group` IN (" + ids + ")\n";
			sql += "			)";
		}

		sql += "	)";

		QScopedPointer<AQuery> query_moderate(createQuery(sql, false));

		if (query_moderate.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		if (query_moderate->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_moderate->getLastError());
		}

		//
		// СѓРґР°Р»РµРЅРёРµ РёР· СЃРїРёСЃРєР° СЃРѕРѕР±С‰РµРЅРёР№
		//

		sql  = "";
		sql += "DELETE FROM\n";
		sql += "	`message`\n";
		sql += "WHERE\n";

		if (type == idsForum)
			sql += "	`id_forum` IN (" + ids + ")";
		else
		{
			sql += "	`id_forum` IN\n";
			sql += "	(\n";
			sql += "		SELECT\n";
			sql += "			`id`\n";
			sql += "		FROM\n";
			sql += "			`forum`\n";
			sql += "		WHERE\n";
			sql += "			`id_group` IN (" + ids + ")\n";
			sql += "	)";
		}

		QScopedPointer<AQuery> query_message(createQuery(sql, false));

		if (query_message.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		if (query_message->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_message->getLastError());
		}

		//
		// СѓРґР°Р»РµРЅРёРµ РёР· СЃРїРёСЃРєР° С‚РѕРїРёРєРѕРІ, С‚.Рє. FOREIGN KEY РЅРµ РѕР±СЂР°Р±Р°С‚С‹РІР°СЋС‚СЃСЏ SQLite (С…РѕС‚СЏ Рё РїРѕР·РІРѕР»СЏСЋС‚ СЃРµР±СЏ СѓРєР°Р·С‹РІР°С‚СЊ)
		//

		sql  = "";
		sql += "DELETE FROM\n";
		sql += "	`message_topic`\n";
		sql += "WHERE\n";

		if (type == idsForum)
			sql += "	`id_forum` IN (" + ids + ")";
		else
		{
			sql += "	`id_forum` IN\n";
			sql += "	(\n";
			sql += "		SELECT\n";
			sql += "			`id`\n";
			sql += "		FROM\n";
			sql += "			`forum`\n";
			sql += "		WHERE\n";
			sql += "			`id_group` IN (" + ids + ")\n";
			sql += "	)";
		}

		QScopedPointer<AQuery> query_message_topic(createQuery(sql, false));

		if (query_message_topic.isNull() == true)
		{
			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		if (query_message_topic->exec() == false)
		{
			ASQLiteDatabase::rollback();

			return returnError(query_message_topic->getLastError());
		}
	}

	//
	// СѓРґР°Р»РµРЅРёРµ РёР· РїРѕРґРїРёСЃР°РЅС‹С…
	//

	sql  = "";
	sql += "DELETE FROM\n";
	sql += "	`subscribed`\n";
	sql += "WHERE\n";

	if (type == idsForum)
		sql += "	`id_forum` IN (" + ids + ")";
	else
	{
		sql += "	`id_forum` IN\n";
		sql += "	(\n";
		sql += "		SELECT\n";
		sql += "			`id`\n";
		sql += "		FROM\n";
		sql += "			`forum`\n";
		sql += "		WHERE\n";
		sql += "			`id_group` IN (" + ids + ")\n";
		sql += "	)";
	}

	QScopedPointer<AQuery> query_subscribed(createQuery(sql, false));

	if (query_subscribed.isNull() == true)
	{
		QString msg = ASQLiteDatabase::getLastError();

		ASQLiteDatabase::rollback();

		return returnError(msg);
	}

	if (query_subscribed->exec() == false)
	{
		ASQLiteDatabase::rollback();

		return returnError(query_subscribed->getLastError());
	}

	//
	// Р·Р°РІРµСЂС€РµРЅРёРµ С‚СЂР°РЅР·Р°РєС†РёРё
	//

	if (ASQLiteDatabase::commit() == false)
		return returnError(ASQLiteDatabase::getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMessageIds (int from_id, int to_id, QList<int>& list)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`id`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id` >= " + QString::number(from_id) + " AND\n";
	sql += "	`id` <  " + QString::number(to_id) + "\n";
	sql += "ORDER BY\n";
	sql += "	`id`";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
		list.append(query_select->value(0).toInt());

	return returnSuccess();

}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::addMessage2Send (const AMessage2Send& info)
{
	QString sql;

	sql += "REPLACE INTO `message2send`\n";
	sql += "(\n";
	sql += "	`id`,\n";
	sql += "	`id_parent`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`subject`,\n";
	sql += "	`message`,\n";
	sql += "	`date`,\n";
	sql += "	`draft`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id,\n";
	sql += "	:id_parent,\n";
	sql += "	:id_forum,\n";
	sql += "	:subject,\n";
	sql += "	:message,\n";
	sql += "	:date,\n";
	sql += "	:draft\n";
	sql += ")";

	QScopedPointer<AQuery> query(createQuery(sql));

	if (query.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query->bindValue(":id",        info.ID);
	query->bindValue(":id_parent", info.IDParent);
	query->bindValue(":id_forum",  info.IDForum);
	query->bindValue(":subject",   info.Subject);
	query->bindValue(":message",   info.Message);
	query->bindValue(":date",      info.Date.toSecsSinceEpoch());
	query->bindValue(":draft",     (info.Draft == true ? 1 : 0)); // РёРЅР°С‡Рµ РєРѕРЅРІРµСЂС‚РёС‚СЃСЏ РІ С‚РµРєСЃС‚РѕРІСѓСЋ СЃС‚СЂРѕРєСѓ "true" / "false"

	if (query->exec() == false)
		return returnError(query->getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMessage2SendList (AMessageInfoList& list, bool drafts)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`id_parent`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`subject`,\n";
	sql += "	`message`,\n";
	sql += "	`date`\n";
	sql += "FROM\n";
	sql += "	`message2send`\n";
	sql += "WHERE\n";
	sql += (QString)"	`draft` " + (drafts == true ? " > 0" : "= 0") + "\n";
	sql += "ORDER BY\n";
	sql += "	`date` DESC";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		AMessageInfo info;

		info.ID             = query_select->value(0).toInt();
		info.IDTopic        = 0;
		info.IDParent       = query_select->value(1).toInt();
		info.IDUser         = 0;
		info.IDForum        = query_select->value(2).toInt();
		info.Subject        = query_select->value(3).toString();
		info.MessageName    = "";
		info.UserNick       = "";
		info.Message        = query_select->value(4).toString();
		info.IDArticle      = 0;
		info.MessageDate    = QDateTime::fromSecsSinceEpoch(query_select->value(5).toUInt());
		info.UpdateDate     = QDateTime();
		info.UserRole       = "";
		info.UserTitle      = "";
		info.UserTitleColor = 0;
		info.LastModerated  = QDateTime();

		// РІСЂРµРјРµРЅРЅС‹Р№ С‚Р°РіР»Р°Р№РЅ
		QString tagline = getTagline();

		if (tagline.length() != 0)
			info.Message = info.Message.trimmed() + "\n\n[tagline]" + tagline + "[/tagline]";

		list.append(info);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::addRating2Send (const ARating2Send& info)
{
	QString sql;

	sql += "REPLACE INTO `rating2send`\n";
	sql += "(\n";
	sql += "	`id`,\n";
	sql += "	`id_message`,\n";
	sql += "	`rate`,\n";
	sql += "	`date`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id,\n";
	sql += "	:id_message,\n";
	sql += "	:rate,\n";
	sql += "	:date\n";
	sql += ")";

	QScopedPointer<AQuery> query(createQuery(sql));

	if (query.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query->bindValue(":id",         info.ID);
	query->bindValue(":id_message", info.IDMessage);
	query->bindValue(":rate",       info.Rate);
	query->bindValue(":date",       info.Date.toSecsSinceEpoch());

	if (query->exec() == false)
		return returnError(query->getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getRating2SendList (AMessageInfoList& message_list, ARating2SendList& rating_list)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`message`.`id`,\n";
	sql += "	`message`.`id_topic`,\n";
	sql += "	`message`.`id_parent`,\n";
	sql += "	`message`.`id_user`,\n";
	sql += "	`message`.`id_forum`,\n";
	sql += "	`message`.`subject`,\n";
	sql += "	`message`.`message_name`,\n";
	sql += "	`message`.`user_nick`,\n";
	sql += "	`message`.`id_article`,\n";
	sql += "	`message`.`message_date`,\n";
	sql += "	`message`.`update_date`,\n";
	sql += "	`message`.`user_role`,\n";
	sql += "	`message`.`user_title`,\n";
	sql += "	`message`.`user_title_color`,\n";
	sql += "	`message`.`last_moderated`,\n";
	sql += "	`message`.`closed`,\n";
	sql += "	`rating2send`.`id`,\n";
	sql += "	`rating2send`.`rate`,\n";
	sql += "	`rating2send`.`date`\n";
	sql += "FROM\n";
	sql += "	`rating2send`\n";
	sql += "		INNER JOIN\n";
	sql += "	`message`\n";
	sql += "		ON\n";
	sql += "	`rating2send`.`id_message` = `message`.`id`\n";
	sql += "ORDER BY\n";
	sql += "	`rating2send`.`date` DESC";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		AMessageInfo message_info;

		message_info.ID             = query_select->value(0).toInt();
		message_info.IDTopic        = query_select->value(1).toInt();
		message_info.IDParent       = query_select->value(2).toInt();
		message_info.IDUser         = query_select->value(3).toInt();
		message_info.IDForum        = query_select->value(4).toInt();
		message_info.Subject        = query_select->value(5).toString();
		message_info.MessageName    = query_select->value(6).toString();
		message_info.UserNick       = query_select->value(7).toString();

		if (getMessageBody(message_info.ID, message_info.Message) == false)
			return false;

		message_info.IDArticle      = query_select->value(8).toInt();
		message_info.MessageDate    = QDateTime::fromSecsSinceEpoch(query_select->value(9).toUInt());
		message_info.UpdateDate     = QDateTime::fromSecsSinceEpoch(query_select->value(10).toUInt());
		message_info.UserRole       = query_select->value(11).toString();
		message_info.UserTitle      = query_select->value(12).toString();
		message_info.UserTitleColor = query_select->value(13).toInt();
		message_info.LastModerated  = QDateTime::fromSecsSinceEpoch(query_select->value(14).toUInt());
		message_info.Closed         = query_select->value(15).toInt();

		message_list.append(message_info);

		ARating2Send rating_info;

		rating_info.ID        = query_select->value(16).toInt();
		rating_info.IDMessage = message_info.ID;
		rating_info.Rate      = query_select->value(17).toInt();
		rating_info.Date      = QDateTime::fromSecsSinceEpoch(query_select->value(18).toUInt());

		rating_list.append(rating_info);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getRating2SendList (ARating2SendList& list)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`id_message`,\n";
	sql += "	`rate`,\n";
	sql += "	`date`\n";
	sql += "FROM\n";
	sql += "	`rating2send`\n";
	sql += "ORDER BY\n";
	sql += "	`date` DESC";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		ARating2Send rating_info;

		rating_info.ID        = query_select->value(0).toInt();
		rating_info.IDMessage = query_select->value(1).toInt();
		rating_info.Rate      = query_select->value(2).toInt();
		rating_info.Date      = QDateTime::fromSecsSinceEpoch(query_select->value(3).toUInt());

		list.append(rating_info);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::changeRating (int id, int new_rate)
{
	QString sql;

	sql += "UPDATE\n";
	sql += "	`rating2send`\n";
	sql += "SET\n";
	sql += "	`rate` = " + QString::number(new_rate) + ",\n";
	sql += "	`date` = " + QString::number(QDateTime::currentDateTime().toSecsSinceEpoch()) + "\n";
	sql += "WHERE\n";
	sql += "	`id` = " + QString::number(id);

	QScopedPointer<AQuery> query_update(createQuery(sql, false));

	if (query_update.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_update->exec() == false)
		return returnError(query_update->getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::addModerate2Send (const AModerate2Send& info)
{
	QString sql;

	sql += "REPLACE INTO `moderate2send`\n";
	sql += "(\n";
	sql += "	`id`,\n";
	sql += "	`id_message`,\n";
	sql += "	`action`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`description`,\n";
	sql += "	`as_moderator`,\n";
	sql += "	`date`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id,\n";
	sql += "	:id_message,\n";
	sql += "	:action,\n";
	sql += "	:id_forum,\n";
	sql += "	:description,\n";
	sql += "	:as_moderator,\n";
	sql += "	:date\n";
	sql += ")";

	QScopedPointer<AQuery> query(createQuery(sql));

	if (query.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query->bindValue(":id",           info.ID);
	query->bindValue(":id_message",   info.IDMessage);
	query->bindValue(":action",       info.Action);
	query->bindValue(":id_forum",     info.IDForum);
	query->bindValue(":description",  info.Description);
	query->bindValue(":as_moderator", info.AsModerator);
	query->bindValue(":date",         info.Date.toSecsSinceEpoch());

	if (query->exec() == false)
		return returnError(query->getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getModerate2SendList (AMessageInfoList& message_list, AModerate2SendList& moderate_list)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`message`.`id`,\n";
	sql += "	`message`.`id_topic`,\n";
	sql += "	`message`.`id_parent`,\n";
	sql += "	`message`.`id_user`,\n";
	sql += "	`message`.`id_forum`,\n";
	sql += "	`message`.`subject`,\n";
	sql += "	`message`.`message_name`,\n";
	sql += "	`message`.`user_nick`,\n";
	sql += "	`message`.`id_article`,\n";
	sql += "	`message`.`message_date`,\n";
	sql += "	`message`.`update_date`,\n";
	sql += "	`message`.`user_role`,\n";
	sql += "	`message`.`user_title`,\n";
	sql += "	`message`.`user_title_color`,\n";
	sql += "	`message`.`last_moderated`,\n";
	sql += "	`message`.`closed`,\n";
	sql += "	`moderate2send`.`id`,\n";
	sql += "	`moderate2send`.`action`,\n";
	sql += "	`moderate2send`.`id_forum`,\n";
	sql += "	`moderate2send`.`description`,\n";
	sql += "	`moderate2send`.`as_moderator`,\n";
	sql += "	`moderate2send`.`date`\n";
	sql += "FROM\n";
	sql += "	`moderate2send`\n";
	sql += "		INNER JOIN\n";
	sql += "	`message`\n";
	sql += "		ON\n";
	sql += "	`moderate2send`.`id_message` = `message`.`id`\n";
	sql += "ORDER BY\n";
	sql += "	`moderate2send`.`date` DESC";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		AMessageInfo message_info;

		message_info.ID             = query_select->value(0).toInt();
		message_info.IDTopic        = query_select->value(1).toInt();
		message_info.IDParent       = query_select->value(2).toInt();
		message_info.IDUser         = query_select->value(3).toInt();
		message_info.IDForum        = query_select->value(4).toInt();
		message_info.Subject        = query_select->value(5).toString();
		message_info.MessageName    = query_select->value(6).toString();
		message_info.UserNick       = query_select->value(7).toString();

		if (getMessageBody(message_info.ID, message_info.Message) == false)
			return false;

		message_info.IDArticle      = query_select->value(8).toInt();
		message_info.MessageDate    = QDateTime::fromSecsSinceEpoch(query_select->value(9).toUInt());
		message_info.UpdateDate     = QDateTime::fromSecsSinceEpoch(query_select->value(10).toUInt());
		message_info.UserRole       = query_select->value(11).toString();
		message_info.UserTitle      = query_select->value(12).toString();
		message_info.UserTitleColor = query_select->value(13).toInt();
		message_info.LastModerated  = QDateTime::fromSecsSinceEpoch(query_select->value(14).toUInt());
		message_info.Closed         = query_select->value(15).toInt();

		message_list.append(message_info);

		AModerate2Send moderate_info;

		moderate_info.ID          = query_select->value(16).toInt();
		moderate_info.IDMessage   = message_info.ID;
		moderate_info.Action      = query_select->value(17).toString();
		moderate_info.IDForum     = query_select->value(18).toInt();
		moderate_info.Description = query_select->value(19).toString();
		moderate_info.AsModerator = query_select->value(20).toInt();
		moderate_info.Date        = QDateTime::fromSecsSinceEpoch(query_select->value(21).toUInt());

		moderate_list.append(moderate_info);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getModerate2SendList (AModerate2SendList& list)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`id_message`,\n";
	sql += "	`action`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`description`,\n";
	sql += "	`as_moderator`,\n";
	sql += "	`date`\n";
	sql += "FROM\n";
	sql += "	`moderate2send`\n";
	sql += "ORDER BY\n";
	sql += "	`date` DESC";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		AModerate2Send moderate_info;

		moderate_info.ID          = query_select->value(0).toInt();
		moderate_info.IDMessage   = query_select->value(1).toInt();
		moderate_info.Action      = query_select->value(2).toString();
		moderate_info.IDForum     = query_select->value(3).toInt();
		moderate_info.Description = query_select->value(4).toString();
		moderate_info.AsModerator = query_select->value(5).toInt();
		moderate_info.Date        = QDateTime::fromSecsSinceEpoch(query_select->value(6).toUInt());

		list.append(moderate_info);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getModerate2SendInfo (int id, AModerate2Send& info)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`id_message`,\n";
	sql += "	`action`,\n";
	sql += "	`id_forum`,\n";
	sql += "	`description`,\n";
	sql += "	`as_moderator`,\n";
	sql += "	`date`\n";
	sql += "FROM\n";
	sql += "	`moderate2send`\n";
	sql += "WHERE\n";
	sql += "	`id` = " + QString::number(id);

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	info.ID = -1; // РґР»СЏ РїСЂРѕРІРµСЂРєРё

	while (query_select->next())
	{
		info.ID          = query_select->value(0).toInt();
		info.IDMessage   = query_select->value(1).toInt();
		info.Action      = query_select->value(2).toString();
		info.IDForum     = query_select->value(3).toInt();
		info.Description = query_select->value(4).toString();
		info.AsModerator = query_select->value(5).toInt();
		info.Date        = QDateTime::fromSecsSinceEpoch(query_select->value(6).toUInt());
	}

	if (info.ID == -1)
		return returnError(QString::fromUtf8("Р—Р°РїСЂРѕС€РµРЅРЅР°СЏ РёРЅС„РѕСЂРјР°С†РёСЏ Рѕ РјРѕРґРµСЂРёР»РєРµ РЅРµ РЅР°Р№РґРµРЅР°"));

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::setCommitResult (const ACommitInfo& info)
{
	QString sql;

	if (info.Messages.count() != 0)
	{
		sql += "DELETE FROM\n";
		sql += "	`message2send`\n";
		sql += "WHERE\n";
		sql += "	`id` IN (";

		for (int i = 0; i < info.Messages.count(); i++)
		{
			sql += QString::number(info.Messages[i]);

			if (i != info.Messages.count() - 1)
				sql += ", ";
		}

		sql += ")";

		QScopedPointer<AQuery> query_delete(createQuery(sql, false));

		if (query_delete.isNull() == true)
			return returnError(ASQLiteDatabase::getLastError());

		if (query_delete->exec() == false)
			return returnError(query_delete->getLastError());
	}

	if (info.Rating.count() != 0)
	{
		sql  = "";
		sql += "DELETE FROM\n";
		sql += "	`rating2send`\n";
		sql += "WHERE\n";
		sql += "	`id` IN (";

		for (int i = 0; i < info.Rating.count(); i++)
		{
			sql += QString::number(info.Rating[i]);

			if (i != info.Rating.count() - 1)
				sql += ", ";
		}

		sql += ")";

		QScopedPointer<AQuery> query_delete(createQuery(sql, false));

		if (query_delete.isNull() == true)
			return returnError(ASQLiteDatabase::getLastError());

		if (query_delete->exec() == false)
			return returnError(query_delete->getLastError());
	}

	if (info.Moderate.count() != 0)
	{
		sql  = "";
		sql += "DELETE FROM\n";
		sql += "	`moderate2send`\n";
		sql += "WHERE\n";
		sql += "	`id` IN (";

		for (int i = 0; i < info.Moderate.count(); i++)
		{
			sql += QString::number(info.Moderate[i]);

			if (i != info.Moderate.count() - 1)
				sql += ", ";
		}

		sql += ")";

		QScopedPointer<AQuery> query_delete(createQuery(sql, false));

		if (query_delete.isNull() == true)
			return returnError(ASQLiteDatabase::getLastError());

		if (query_delete->exec() == false)
			return returnError(query_delete->getLastError());
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::deleteSpecial (const QList<int>& ids, int id_special)
{
	if (ids.count() == 0)
		return returnSuccess();

	QString sql;

	sql += "DELETE FROM\n";

	if (id_special == SPECIAL_ID_FORUM_MESSAGE2SEND || id_special == SPECIAL_ID_FORUM_DRAFTS)
		sql += "	`message2send`\n";
	else if (id_special == SPECIAL_ID_FORUM_RATING2SEND)
		sql += "	`rating2send`\n";
	else if (id_special == SPECIAL_ID_FORUM_MODERATE2SEND)
		sql += "	`moderate2send`\n";
	else
		return returnError(QString::fromUtf8("РќРµРёР·РІРµСЃС‚РЅС‹Р№ СЃРїРµС†-С„РѕСЂСѓРј [") + QString::number(id_special) + "]");

	sql += "WHERE\n";
	sql += "	`id` IN (";

	for (int i = 0; i < ids.count(); i++)
	{
		sql += QString::number(ids[i]);

		if (i < ids.count() - 1)
			sql += ", ";
	}

	sql += ")";

	QScopedPointer<AQuery> query_delete(createQuery(sql, false));

	if (query_delete.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_delete->exec() == false)
		return returnError(query_delete->getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMessageRatingList (int id_message, AMessageRatingList& list)
{
	QString sql;

	sql += "SELECT\n";
	sql += "	`rating`.`id_user`,\n";
	sql += "	`user`.`name`,\n";
	sql += "	`user`.`nick`,\n";
	sql += "	`rating`.`user_rating`,\n";
	sql += "	`rating`.`rate`\n";
	sql += "FROM\n";
	sql += "	`rating`\n";
	sql += "		INNER JOIN\n";
	sql += "	`user`\n";
	sql += "		ON\n";
	sql += "	`rating`.`id_user` = `user`.`id`\n";
	sql += "WHERE\n";
	sql += "	`rating`.`id_message` = :id_message\n";
	sql += "ORDER BY\n";
	sql += "	`rating`.`rate_date` DESC";

	QScopedPointer<AQuery> query_select(createQuery(sql));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->bindValue(":id_message", id_message);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
	{
		AMessageRating info;

		info.IDUser     = query_select->value(0).toInt();
		info.Name       = query_select->value(1).toString();
		info.Nick       = query_select->value(2).toString();
		info.UserRating = query_select->value(3).toInt();
		info.Rate       = query_select->value(4).toInt();

		list.append(info);
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::compressStorage (IProgress* progress)
{
	if (progress != NULL)
		progress->onProgress(0);

	#ifndef AVALON_USE_ZLIB
	return returnError(QString::fromUtf8("СЃРєРѕРјРїРёР»РёСЂСѓР№С‚Рµ avalon СЃ РїРѕРґРґРµСЂР¶РєРѕР№ zlib"));
	#else

	QString sql;

	//
	// Р·Р°РіРѕС‚РѕРІРєРё Р·Р°РїСЂРѕСЃРѕРІ
	//

	// Р·Р°РїСЂРѕСЃ РЅР° РїРѕР»СѓС‡РµРЅРёРµ СЃС‚СЂР°РЅРёС†С‹
	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`message`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id` >= :id_1 AND\n";
	sql += "	`id` <  :id_2 AND\n";
	sql += "	`compressed` = 0";

	QScopedPointer<AQuery> query_select(createQuery(sql));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	// Р·Р°РїСЂРѕСЃ РЅР° РѕР±РЅРѕРІР»РµРЅРёРµ РґР°РЅРЅС‹С…
	sql  = "";
	sql += "UPDATE\n";
	sql += "	`message`\n";
	sql += "SET\n";
	sql += "	`message`    = :message,\n";
	sql += "	`compressed` = 1\n";
	sql += "WHERE\n";
	sql += "	`id` = :id";

	QScopedPointer<AQuery> query_update(createQuery(sql));

	if (query_update.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	//
	// РїРµСЂРІРёС‡РЅС‹Рµ Р·РЅР°С‡РµРЅРёСЏ
	//

	// РїРѕР»СѓС‡РµРЅРёРµ РјР°РєСЃРёРјР°Р»СЊРЅРѕРіРѕ Рё РјРёРЅРёРјР°Р»СЊРЅРѕРіРѕ ID
	int min_id = 0;
	int max_id = 0;

	if (getMinIDMessage(min_id) == false)
		return false;

	if (getMaxIDMessage(max_id) == false)
		return false;

	// РµСЃР»Рё РЅРµС‡РµРіРѕ СЃР¶РёРјР°С‚СЊ
	if (max_id - min_id == 0)
	{
		if (progress != NULL)
			progress->onProgress(100, QString::fromUtf8("РЎР¶Р°С‚РёРµ С…СЂР°РЅРёР»РёС‰Р° Р·Р°РІРµСЂС€РµРЅРѕ СѓСЃРїРµС€РЅРѕ!"));

		return returnSuccess();
	}

	// Р·Р°РіРѕС‚РѕРІРєР° РїР°РјСЏС‚Рё РґР»СЏ СЃР¶Р°С‚РёСЏ СЃРѕРѕР±С‰РµРЅРёР№
	uLongf compressed_buf_size      = 3 * 1024 * 1024;                // СЂР°Р·РјРµСЂ Р±СѓС„РµСЂР° РґР»СЏ СЃР¶Р°С‚С‹С… РґР°РЅРЅС‹С… (РјРѕР¶РµС‚ РјРµРЅСЏС‚СЊСЃСЏ РїСЂРё РЅРµРѕР±С…РѕРґРёРјРѕСЃС‚Рё)
	Bytef* compressed_buf           = new Bytef[compressed_buf_size]; // Р±СѓС„РµСЂ РґР»СЏ СЃР¶Р°С‚С‹С… РґР°РЅРЅС‹С…
	uLongf compressed_buf_size_real = 0;                              // СЂРµР°Р»СЊРЅР°СЏ РґР»РёРЅР° СЃР¶Р°С‚С‹С… РґР°РЅРЅС‹С…

	// РїР°СЂР°РјРµС‚СЂС‹ РїРѕСЃС‚СЂР°РЅРёС‡РЅРѕР№ РѕР±СЂР°Р±РѕС‚РєРё
	int page = 1000;
	int id_1 = min_id;
	int id_2 = id_1 + page;

	//
	// СЃР¶Р°С‚РёРµ Р±Р»РѕРєР°РјРё РїРѕ СЃС‚СЂР°РЅРёС†Рµ
	//

	while (id_1 <= max_id)
	{
		// СЃС‚СЂР°РЅРёС†Сѓ РѕРїС‚РёРјР°Р»СЊРЅРµРµ РѕР±СЂР°Р±Р°С‚С‹РІР°С‚СЊ РІ С‚СЂР°РЅР·Р°РєС†РёРё
		if (ASQLiteDatabase::transaction() == false)
		{
			delete [] compressed_buf;
			return returnError(ASQLiteDatabase::getLastError());
		}

		// РїРѕР»СѓС‡РµРЅРёРµ СЃС‚СЂР°РЅРёС†С‹
		query_select->bindValue(":id_1", id_1);
		query_select->bindValue(":id_2", id_2);

		if (query_select->exec() == false)
		{
			delete [] compressed_buf;

			ASQLiteDatabase::rollback();

			return returnError(query_select->getLastError());
		}

		//
		// РѕР±СЂР°Р±РѕС‚РєР° СЃС‚СЂР°РЅРёС†С‹
		//

		while (query_select->next())
		{
			int        id      = query_select->value(0).toInt();
			QByteArray message = query_select->value(1).toString().toUtf8();

			uLongf       source_buf_size = message.count();
			const Bytef* source_buf      = (const Bytef*)message.constData();

			// РїСЂРѕРІРµСЂРєР° РЅР° РґРѕСЃС‚Р°С‚РѕС‡РЅСѓСЋ СЂР°Р·РјРµСЂРЅРѕСЃС‚СЊ Р±СѓС„РµСЂР° СЃР¶Р°С‚РёСЏ
			if (source_buf_size * 2 > compressed_buf_size)
			{
				delete [] compressed_buf;
				compressed_buf_size = source_buf_size * 2;
				compressed_buf = new Bytef[compressed_buf_size];
			}

			compressed_buf_size_real = compressed_buf_size;

			// СЃР¶Р°С‚РёРµ (СЃРј. РґРѕРєСѓРјРµРЅС‚Р°С†РёСЋ РїРѕ zlib)
			int z_result = compress2(compressed_buf, &compressed_buf_size_real, source_buf, source_buf_size, 9);

			// Р°РЅР°Р»РёР· СЂРµР·СѓР»СЊС‚Р°С‚Р°, РµСЃР»Рё РїСЂРѕРёР·РѕС€Р»Р° РѕС€РёР±РєР° РёР»Рё СЂР°Р·РјРµСЂ СЃР¶Р°С‚С‹С… РґР°РЅРЅС‹С… Р±РѕР»СЊС€Рµ РёСЃС…РѕРґРЅС‹С…
			bool compressed;

			if (z_result != Z_OK)
				compressed = false;
			else if (compressed_buf_size_real >= source_buf_size)
				compressed = false;
			else
				compressed = true;

			if (compressed == true)
			{
				query_update->bindValue(":id",      id);
				query_update->bindValue(":message", QByteArray(reinterpret_cast<const char*>(compressed_buf), compressed_buf_size_real));

				if (query_update->exec() == false)
				{
					delete [] compressed_buf;

					ASQLiteDatabase::rollback();

					return returnError(query_update->getLastError());
				}
			}

		}   // while (query_select->next())

		if (ASQLiteDatabase::commit() == false)
		{
			delete [] compressed_buf;

			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		// РІС‹С‡РёСЃР»РµРЅРёРµ РЅРѕРІРѕР№ СЃС‚СЂР°РЅРёС†С‹
		id_1 = id_2;
		id_2 = id_1 + page;

		// РїСЂРѕРіСЂРµСЃСЃ РѕРїРµСЂР°С†РёРё
		if (progress != NULL)
			progress->onProgress(id_1 * 100 / max_id);

	}   // while (id_1 <= max_id)

	delete [] compressed_buf;

	if (progress != NULL)
	{
		progress->onProgress(100, QString::fromUtf8("РЎР¶Р°С‚РёРµ С…СЂР°РЅРёР»РёС‰Р° Р·Р°РІРµСЂС€РµРЅРѕ СѓСЃРїРµС€РЅРѕ!"));
		progress->onProgress(100, QString::fromUtf8(" "));
		progress->onProgress(100, QString::fromUtf8("Р”Р»СЏ СѓРјРµРЅСЊС€РµРЅРёСЏ С„СЂР°РіРјРµРЅС‚Р°С†РёРё РґР°РЅРЅС‹С…"));
		progress->onProgress(100, QString::fromUtf8("РІС‹РїРѕР»РЅРёС‚Рµ РєРѕРјР°РЅРґС‹ VACUUM Рё ANALYZE."));
	}

	return returnSuccess();
	#endif   // #ifdef AVALON_USE_ZLIB
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::uncompressStorage (IProgress* progress)
{
	if (progress != NULL)
		progress->onProgress(0);

	#ifndef AVALON_USE_ZLIB
	return returnError(QString::fromUtf8("СЃРєРѕРјРїРёР»РёСЂСѓР№С‚Рµ avalon СЃ РїРѕРґРґРµСЂР¶РєРѕР№ zlib"));
	#else

	QString sql;

	//
	// Р·Р°РіРѕС‚РѕРІРєРё Р·Р°РїСЂРѕСЃРѕРІ
	//

	// Р·Р°РїСЂРѕСЃ РЅР° РїРѕР»СѓС‡РµРЅРёРµ СЃС‚СЂР°РЅРёС†С‹
	sql += "SELECT\n";
	sql += "	`id`,\n";
	sql += "	`message`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id` >= :id_1 AND\n";
	sql += "	`id` <  :id_2 AND\n";
	sql += "	`compressed` = 1";

	QScopedPointer<AQuery> query_select(createQuery(sql));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	// Р·Р°РїСЂРѕСЃ РЅР° РѕР±РЅРѕРІР»РµРЅРёРµ РґР°РЅРЅС‹С…
	sql  = "";
	sql += "UPDATE\n";
	sql += "	`message`\n";
	sql += "SET\n";
	sql += "	`message`    = :message,\n";
	sql += "	`compressed` = 0\n";
	sql += "WHERE\n";
	sql += "	`id` = :id";

	QScopedPointer<AQuery> query_update(createQuery(sql));

	if (query_update.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	//
	// РїРµСЂРІРёС‡РЅС‹Рµ Р·РЅР°С‡РµРЅРёСЏ
	//

	// РїРѕР»СѓС‡РµРЅРёРµ РјР°РєСЃРёРјР°Р»СЊРЅРѕРіРѕ Рё РјРёРЅРёРјР°Р»СЊРЅРѕРіРѕ ID
	int min_id = 0;
	int max_id = 0;

	if (getMinIDMessage(min_id) == false)
		return false;

	if (getMaxIDMessage(max_id) == false)
		return false;

	// РµСЃР»Рё РЅРµС‡РµРіРѕ СЃР¶РёРјР°С‚СЊ
	if (max_id - min_id == 0)
	{
		if (progress != NULL)
			progress->onProgress(100, QString::fromUtf8("Р Р°СЃРїР°РєРѕРІРєР° С…СЂР°РЅРёР»РёС‰Р° Р·Р°РІРµСЂС€РµРЅР° СѓСЃРїРµС€РЅРѕ!"));

		return returnSuccess();
	}

	// Р·Р°РіРѕС‚РѕРІРєР° РїР°РјСЏС‚Рё РґР»СЏ СЂР°СЃРїР°РєРѕРІРєРё СЃРѕРѕР±С‰РµРЅРёР№
	uLongf uncompressed_buf_size      = 3 * 1024 * 1024;                  // СЂР°Р·РјРµСЂ СЂР°СЃРїР°РєРѕРІР°РЅРЅС‹С… РґР°РЅРЅС‹С… (РјРµРЅСЏРµС‚СЃСЏ РїСЂРё РЅРµРѕР±С…РѕРґРёРјРѕСЃС‚Рё)
	Bytef* uncompressed_buf           = new Bytef[uncompressed_buf_size]; // Р±СѓС„РµСЂ СЂР°СЃРїР°РєРѕРІР°РЅРЅС‹С… РґР°РЅРЅС‹С…
	uLongf uncompressed_buf_size_real = uncompressed_buf_size;            // СЂРµР°Р»СЊРЅС‹Р№ СЂР°Р·РјРµСЂ СЂР°СЃРїР°РєРѕРІР°РЅРЅС‹С… РґР°РЅРЅС‹С…

	// РїР°СЂР°РјРµС‚СЂС‹ РїРѕСЃС‚СЂР°РЅРёС‡РЅРѕР№ РѕР±СЂР°Р±РѕС‚РєРё
	int page = 1000;
	int id_1 = min_id;
	int id_2 = id_1 + page;

	//
	// СЃР¶Р°С‚РёРµ Р±Р»РѕРєР°РјРё РїРѕ СЃС‚СЂР°РЅРёС†Рµ
	//

	while (id_1 <= max_id)
	{
		// СЃС‚СЂР°РЅРёС†Сѓ РѕРїС‚РёРјР°Р»СЊРЅРµРµ РѕР±СЂР°Р±Р°С‚С‹РІР°С‚СЊ РІ С‚СЂР°РЅР·Р°РєС†РёРё
		if (ASQLiteDatabase::transaction() == false)
		{
			delete [] uncompressed_buf;
			return returnError(ASQLiteDatabase::getLastError());
		}

		// РїРѕР»СѓС‡РµРЅРёРµ СЃС‚СЂР°РЅРёС†С‹
		query_select->bindValue(":id_1", id_1);
		query_select->bindValue(":id_2", id_2);

		if (query_select->exec() == false)
		{
			delete [] uncompressed_buf;

			ASQLiteDatabase::rollback();

			return returnError(query_select->getLastError());
		}

		//
		// РѕР±СЂР°Р±РѕС‚РєР° СЃС‚СЂР°РЅРёС†С‹
		//

		while (query_select->next())
		{
			int        id      = query_select->value(0).toInt();
			QByteArray message = query_select->value(1).toByteArray();

			uLongf       compressed_buf_size = message.count();
			const Bytef* compressed_buf      = (const Bytef*)message.constData();

			uncompressed_buf_size_real = uncompressed_buf_size;

			// СЂР°СЃРїР°РєРѕРІРєР°
			while (true)
			{
				int z_result = uncompress(uncompressed_buf, &uncompressed_buf_size_real, compressed_buf, compressed_buf_size);

				if (z_result == Z_OK)
				{
					QString body = QString::fromUtf8(reinterpret_cast<const char*>(uncompressed_buf), uncompressed_buf_size_real);

					query_update->bindValue(":id",      id);
					query_update->bindValue(":message", body);

					if (query_update->exec() == false)
					{
						delete [] uncompressed_buf;

						ASQLiteDatabase::rollback();

						return returnError(query_update->getLastError());
					}

					break;
				}
				else if (z_result == Z_BUF_ERROR)
				{
					// РЅРµ С…РІР°С‚РёР»Рѕ РґР»РёРЅС‹ Р±СѓС„РµСЂР° РґР»СЏ СЂР°СЃРїР°РєРѕРІРєРё
					delete [] uncompressed_buf;

					uncompressed_buf_size     *= 2;
					uncompressed_buf           = new Bytef[uncompressed_buf_size];
					uncompressed_buf_size_real = uncompressed_buf_size;

					continue;
				}

				delete [] uncompressed_buf;

				ASQLiteDatabase::rollback();

				// РїСЂРѕС‡РёРµ С„Р°С‚Р°Р»СЊРЅС‹Рµ РѕС€РёР±РєРё
				if (z_result == Z_MEM_ERROR)
					return returnError("Z_MEM_ERROR: there was not enough memory.");
				else if (z_result == Z_DATA_ERROR)
					return returnError("Z_DATA_ERROR: the input data was corrupted.");
				else
					return returnError("Z_UNKNOWN_ERROR: z_result = " + QString::number(z_result));

			}   // while (true)

		}   // while (query_select->next())

		if (ASQLiteDatabase::commit() == false)
		{
			delete [] uncompressed_buf;

			QString msg = ASQLiteDatabase::getLastError();

			ASQLiteDatabase::rollback();

			return returnError(msg);
		}

		// РІС‹С‡РёСЃР»РµРЅРёРµ РЅРѕРІРѕР№ СЃС‚СЂР°РЅРёС†С‹
		id_1 = id_2;
		id_2 = id_1 + page;

		// РїСЂРѕРіСЂРµСЃСЃ РѕРїРµСЂР°С†РёРё
		if (progress != NULL)
			progress->onProgress(id_1 * 100 / max_id);

	}   // while (id_1 <= max_id)

	delete [] uncompressed_buf;

	if (progress != NULL)
	{
		progress->onProgress(100, QString::fromUtf8("Р Р°СЃРїР°РєРѕРІРєР° С…СЂР°РЅРёР»РёС‰Р° Р·Р°РІРµСЂС€РµРЅР° СѓСЃРїРµС€РЅРѕ!"));
		progress->onProgress(100, QString::fromUtf8(" "));
		progress->onProgress(100, QString::fromUtf8("Р”Р»СЏ СѓРјРµРЅСЊС€РµРЅРёСЏ С„СЂР°РіРјРµРЅС‚Р°С†РёРё РґР°РЅРЅС‹С…"));
		progress->onProgress(100, QString::fromUtf8("РІС‹РїРѕР»РЅРёС‚Рµ РєРѕРјР°РЅРґС‹ VACUUM Рё ANALYZE."));
	}

	return returnSuccess();
	#endif   // #ifdef AVALON_USE_ZLIB
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMessagePath (int id_message, int& id_forum, QList<int>& path)
{
	int id_parent = 0;

	id_forum = 0;

	path.clear();

	QString sql;

	// Р·Р°РїСЂРѕСЃ РЅР° РїРѕР»СѓС‡РµРЅРёРµ РїРµСЂРІРёС‡РЅРѕР№ РёРЅС„РѕСЂРјР°С†РёРё
	sql += "SELECT\n";
	sql += "	`id_topic`,\n";
	sql += "	`id_parent`,\n";
	sql += "	`id_forum`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id` = " + QString::number(id_message);

	QScopedPointer<AQuery> query_select_first(createQuery(sql, false));

	if (query_select_first.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_select_first->exec() == false)
		return returnError(query_select_first->getLastError());

	while (query_select_first->next())
	{
		id_parent = query_select_first->value(1).toInt();
		id_forum  = query_select_first->value(2).toInt();

		if (id_forum != 0)
			path.append(id_message);
	}

	// СЃРѕРѕР±С‰РµРЅРёРµ РЅРµ РЅР°Р№РґРµРЅРѕ РёР»Рё РЅРµС‚ СЃРѕРѕР±С‰РµРЅРёР№ РІС‹С€Рµ РїРѕ РґРµСЂРµРІСѓ
	if (id_forum == 0 || id_parent == 0)
		return returnSuccess();

	// Р·Р°РїСЂРѕСЃ РґР»СЏ РїСЂРѕС…РѕРґР° Рє РєРѕСЂРЅСЋ РІРµС‚РєРё
	sql  = "";
	sql += "SELECT\n";
	sql += "	`id_parent`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id` = :id";

	QScopedPointer<AQuery> query_select(createQuery(sql));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	// РїСЂРѕС…РѕРґ РґРѕ РєРѕСЂРЅСЏ
	while (id_parent != 0)
	{
		path.insert(0, id_parent);

		query_select->bindValue(":id", id_parent);

		if (query_select->exec() == false)
			return returnError(query_select->getLastError());

		while (query_select->next())
			id_parent = query_select->value(0).toInt();
	}

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::addBroken (int id_message, bool is_topic)
{
	QString sql;

	sql += "REPLACE INTO `broken`\n";
	sql += "(\n";
	sql += "	`id_message`,\n";
	sql += "	`is_topic`\n";
	sql += ")\n";
	sql += "VALUES\n";
	sql += "(\n";
	sql += "	:id_message,\n";
	sql += "	:is_topic\n";
	sql += ")";

	QScopedPointer<AQuery> query_insert(createQuery(sql));

	if (query_insert.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_insert->bindValue(":id_message", id_message);
	query_insert->bindValue(":is_topic",   (is_topic == true ? 1 : 0));

	if (query_insert->exec() == false)
		return returnError(query_insert->getLastError());

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::hasBroken (bool& result)
{
	result = false;

	QString sql;

	sql += "SELECT\n";
	sql += "	COUNT(*)\n";
	sql += "FROM\n";
	sql += "	`broken`\n";
	sql += "LIMIT\n";
	sql += "	1";

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
		result = (query_select->value(0).toInt() > 0);

	return returnSuccess();
}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getMyMessageList (int id_me, int count, QList<int>& list)
{
	// Р°РЅРѕРЅРёРјС‹ РЅР°Рј РЅРµ РёРЅС‚РµСЂРµСЃРЅС‹
	if (id_me == 0)
		return returnSuccess();

	QString sql;

	sql += "SELECT\n";
	sql += "	`id`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id_user` = " + QString::number(id_me) + "\n";
	sql += "ORDER BY\n";
	sql += "	`id` DESC";

	if (count > 0)
		sql += "\nLIMIT " + QString::number(count);

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
		list.append(query_select->value(0).toInt());

	return returnSuccess();

}
//----------------------------------------------------------------------------------------------

bool ASQLiteStorage::getAnswers2MeList (int id_me, int count, QList<int>& list)
{
	// Р°РЅРѕРЅРёРјС‹ РЅР°Рј РЅРµ РёРЅС‚РµСЂРµСЃРЅС‹
	if (id_me == 0)
		return returnSuccess();

	QString sql;

	sql += "SELECT\n";
	sql += "	`id`\n";
	sql += "FROM\n";
	sql += "	`message`\n";
	sql += "WHERE\n";
	sql += "	`id_parent` IN\n";
	sql += "	(\n";
	sql += "		SELECT\n";
	sql += "			`id`\n";
	sql += "		FROM\n";
	sql += "			`message`\n";
	sql += "		WHERE\n";
	sql += "			`id_user` = " + QString::number(id_me) + "\n";
	sql += "		ORDER BY\n";
	sql += "			`id` DESC\n";
	sql += "	)\n";
	sql += "ORDER BY\n";
	sql += "	`id` DESC";

	if (count > 0)
		sql += "\nLIMIT " + QString::number(count);

	QScopedPointer<AQuery> query_select(createQuery(sql, false));

	if (query_select.isNull() == true)
		return returnError(ASQLiteDatabase::getLastError());

	query_select->setForwardOnly(true);

	if (query_select->exec() == false)
		return returnError(query_select->getLastError());

	while (query_select->next())
		list.append(query_select->value(0).toInt());

	return returnSuccess();

}
//----------------------------------------------------------------------------------------------

