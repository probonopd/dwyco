
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "convmodel.h"
#include "dlli.h"
#include "dwyco_new_msg.h"
#include "getinfo.h"
#include "ignoremodel.h"

void hack_unread_count();

ConvListModel *TheConvListModel;

void
Conversation::load_external_state(const QByteArray& uid)
{
    update_display(dwyco_info_to_display(uid));
    update_is_blocked(dwyco_is_ignored(uid.constData(), uid.length()));
    int reviewed = 0;
    int regular = 0;
    get_review_status(uid, reviewed, regular);
    update_REGULAR(regular);
    update_REVIEWED(reviewed);
    update_unseen_count(uid_unviewed_msgs_count(uid));
    update_any_unread(any_unread_msg(uid));
    update_session_msg(session_msg(uid));
    update_pal(dwyco_is_pal(uid.constData(), uid.length()));
}

ConvListModel::ConvListModel(QObject *parent) :
    QQmlObjectListModel<Conversation>(parent, "display", "uid")
{
    if(TheConvListModel)
        ::abort();
    TheConvListModel = this;

}

ConvListModel::~ConvListModel()
{
    TheConvListModel = 0;
}

static QByteArray
dwyco_get_attr(DWYCO_LIST l, int row, const char *col)
{
    const char *val;
    int len;
    int type;
    if(!dwyco_list_get(l, row, col, &val, &len, &type))
        ::abort();
    if(type != DWYCO_TYPE_STRING && type != DWYCO_TYPE_NIL)
        ::abort();
    return QByteArray(val, len);
}

static int
dwyco_get_attr_int(DWYCO_LIST l, int row, const char *col, int& int_out)
{
    const char *val;
    int len;
    int type;
    if(!dwyco_list_get(l, row, col, &val, &len, &type))
        return 0;
    if(type != DWYCO_TYPE_INT)
        return 0;
    QByteArray str_out = QByteArray(val, len);
    int_out = str_out.toInt();
    return 1;
}


void
init_convlist_model()
{

}

void
ConvListModel::set_all_selected(bool b)
{
    int n = count();
    for(int i = 0; i < n; ++i)
    {
        Conversation *c = at(i);
        c->set_selected(b);
    }

}

bool
ConvListModel::at_least_one_selected()
{
    int n = count();
    for(int i = 0; i < n; ++i)
    {
        Conversation *c = at(i);
        if(c->get_selected())
            return true;
    }
    return false;

}



void
ConvListModel::delete_all_selected()
{
    int n = count();
    QList<Conversation *> to_remove;
    for(int i = 0; i < n; ++i)
    {
        Conversation *c = at(i);
        if(c->get_selected())
        {
            to_remove.append(c);
            QByteArray buid = c->get_uid().toLatin1();
            buid = QByteArray::fromHex(buid);
            if(dwyco_is_pal(buid.constData(), buid.length()))
                continue;
            dwyco_delete_user(buid.constData(), buid.length());
            del_unviewed_uid(buid);
        }
    }

    hack_unread_count();
    dwyco_load_users2(1, 0);
    load_users_to_model();

}

void
ConvListModel::pal_all_selected(bool b)
{
    int n = count();
    for(int i = 0; i < n; ++i)
    {
        Conversation *c = at(i);
        if(c->get_selected())
        {
            QByteArray buid = c->get_uid().toLatin1();
            buid = QByteArray::fromHex(buid);
            if(b)
                dwyco_pal_add(buid.constData(), buid.length());
            else
                dwyco_pal_delete(buid.constData(), buid.length());
            c->update_pal(b);

        }
    }
}

void
ConvListModel::block_all_selected()
{
    int n = count();
    for(int i = 0; i < n; ++i)
    {
        Conversation *c = at(i);
        if(c->get_selected())
        {
            QByteArray buid = c->get_uid().toLatin1();
            buid = QByteArray::fromHex(buid);
            dwyco_ignore(buid.constData(), buid.length());
            TheIgnoreListModel->add_uid_to_model(buid);
            c->update_is_blocked(true);
        }
    }
}

void
ConvListModel::decorate(QString huid, QString txt, QString mid)
{
    QByteArray uid = QByteArray::fromHex(huid.toLatin1());
    Conversation *c = getByUid(huid);
    if(!c)
        return;
    int cnt = uid_unviewed_msgs_count(uid);
    c->update_unseen_count(cnt);
    c->update_any_unread(any_unread_msg(uid));
    c->update_is_blocked(dwyco_is_ignored(uid.constData(), uid.length()));
}

void
ConvListModel::decorate(QString huid)
{
    decorate(huid, "", "");
}

Conversation *
ConvListModel::add_uid_to_model(const QByteArray& uid)
{
    QString huid = uid.toHex();
    Conversation *c = getByUid(huid);
    if(!c)
    {
        c = new Conversation(this);
        c->update_uid(huid);
        append(c);
    }
    c->load_external_state(uid);
    return c;
}

void
ConvListModel::remove_uid_from_model(const QByteArray& uid)
{
    QString huid = uid.toHex();
    Conversation *c = getByUid(huid);
    if(!c)
        return;
    remove(c);
}

void
ConvListModel::load_users_to_model()
{
    DWYCO_LIST l;
    int n;
    static int cnt;
    ++cnt;

    dwyco_get_user_list2(&l, &n);
    for(int i = 0; i < n; ++i)
    {
        QByteArray uid = dwyco_get_attr(l, i, DWYCO_NO_COLUMN);
        Conversation *c = add_uid_to_model(uid);
        c->update_counter = cnt;
    }
    // find removed items
    // there is probably a faster way of doing this, but
    // given this should not happen often or with very long lists,
    // might not be worth the effort
    QList<Conversation *> dead;
    for(int i = 0; i < count(); ++i)
    {
        Conversation *c = at(i);
        if(c->update_counter < cnt)
            dead.append(c);

    }
    for(int i = 0; i < dead.count(); ++i)
    {
        Conversation *c = dead[i];
        remove(c);
    }
    dwyco_list_release(l);
}



void
ConvListModel::uid_resolved(const QString &huid)
{
    Conversation *c = getByUid(huid);
    if(!c)
        return;

    QByteArray buid = QByteArray::fromHex(huid.toLatin1());
    c->update_display(dwyco_info_to_display(buid));
    c->update_invalid(0);
    int regular = 0;
    int reviewed = 0;
    get_review_status(buid, reviewed, regular);
    c->update_REGULAR(regular);
    c->update_REVIEWED(reviewed);
    c->update_resolved_counter(c->get_resolved_counter() + 1);
}

void
ConvListModel::uid_invalidate_profile(const QString &huid)
{
    Conversation *c = getByUid(huid);
    if(!c)
        return;
    c->update_invalid(1);

}

ConvSortFilterModel::ConvSortFilterModel(QObject *p)
    : QSortFilterProxyModel(p)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    sort(0);
    m_count = 0;
}

void
ConvSortFilterModel::toggle_selected(QString uid)
{
    ConvListModel *m = dynamic_cast<ConvListModel *>(sourceModel());
    if(!m)
        ::abort();
    Conversation *c = m->getByUid(uid);
    if(!c)
        return;
    c->set_selected(!c->get_selected());
}

void
ConvSortFilterModel::set_all_selected(bool b)
{
    ConvListModel *m = dynamic_cast<ConvListModel *>(sourceModel());
    if(!m)
        ::abort();
    m->set_all_selected(b);
}

void
ConvSortFilterModel::delete_all_selected()
{
    ConvListModel *m = dynamic_cast<ConvListModel *>(sourceModel());
    if(!m)
        ::abort();
    m->delete_all_selected();

}

void
ConvSortFilterModel::block_all_selected()
{
    ConvListModel *m = dynamic_cast<ConvListModel *>(sourceModel());
    if(!m)
        ::abort();
    m->block_all_selected();

}

void
ConvSortFilterModel::pal_all_selected(bool b)
{
    ConvListModel *m = dynamic_cast<ConvListModel *>(sourceModel());
    if(!m)
        ::abort();
    m->pal_all_selected(b);

}

bool
ConvSortFilterModel::at_least_one_selected()
{
    ConvListModel *m = dynamic_cast<ConvListModel *>(sourceModel());
    if(!m)
        ::abort();
    return m->at_least_one_selected();

}

bool
ConvSortFilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    ConvListModel *m = dynamic_cast<ConvListModel *>(sourceModel());
    if(!m)
        return false;


    bool lsm = m->data(left, m->roleForName("session_msg")).toBool();
    bool rsm = m->data(right, m->roleForName("session_msg")).toBool();
    if(lsm && !rsm)
        return true;
    else if(!lsm && rsm)
        return false;

//    int luc = m->data(left, m->roleForName("unseen_count")).toInt();
//    int ruc = m->data(right, m->roleForName("unseen_count")).toInt();
//    if(luc < ruc)
//        return false;
//    else if(ruc < luc)
//        return true;

    bool lau = m->data(left, m->roleForName("any_unread")).toBool();
    bool rau = m->data(right, m->roleForName("any_unread")).toBool();
    if(lau && !rau)
        return true;
    else if(!lau && rau)
        return false;

    bool lsp = m->data(left, m->roleForName("pal")).toBool();
    bool rsp = m->data(right, m->roleForName("pal")).toBool();
    if(lsp && !rsp)
        return true;
    else if(!lsp && rsp)
        return false;

    bool lreg = m->data(left, m->roleForName("REGULAR")).toBool();
    bool rreg = m->data(right, m->roleForName("REGULAR")).toBool();
    if(lreg && !rreg)
        return true;
    else if(!lreg && rreg)
        return false;

    // put blocked users down at the bottom?
//    bool lreg = m->data(left, m->roleForName("is_blocked")).toBool();
//    bool rreg = m->data(right, m->roleForName("is_blocked")).toBool();
//    if(lreg && !rreg)
//        return false;
//    else if(!lreg && rreg)
//        return true;

    int ret1 = QSortFilterProxyModel::lessThan(left, right);
    int ret2 = QSortFilterProxyModel::lessThan(right, left);
    if(ret1 == 0 && ret2 == 0)
    {
        // stabilize the sort with uid tie breaker
        QString uidl = m->data(left, m->roleForName("uid")).toString();
        QString uidr = m->data(right, m->roleForName("uid")).toString();
        if(uidl < uidr)
            return 1;
        return 0;
    }
    return ret1;
}



