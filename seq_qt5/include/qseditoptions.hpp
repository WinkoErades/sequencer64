#ifndef SEQ64_QSEDITOPTIONS_HPP
#define SEQ64_QSEDITOPTIONS_HPP

/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          qseditoptions.hpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-08-31
 * \license       GNU GPLv2 or above
 *
 */

#include <QDialog>

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace Ui
{
    class qseditoptions;
}

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *
 */

class qseditoptions : public QDialog
{
    Q_OBJECT

public:

    explicit qseditoptions (seq64::perform & perf, QWidget * parent = 0);
    virtual ~qseditoptions();

private:

    // makes sure the dialog properly reflects internal settings

    void syncWithInternals();

    // backup preferences incase we cancel changes

    void backup();

    const perform & perf () const
    {
        return mPerf;
    }

    perform & perf ()
    {
        return mPerf;
    }

private:

    Ui::qseditoptions * ui;

    seq64::perform & mPerf;

    // Backup variables for settings.  Not everything new is currently backed
    // up, though.

    bool backupJackTransport;
    bool backupTimeMaster;
    bool backupMasterCond;
    bool backupNoteResume;
    bool backupJackMidi;
    int  backupKeyHeight;

private slots:

    void update_jack_connect ();
    void update_jack_disconnect ();
    void update_master_cond ();
    void update_time_master ();
    void update_transport_support ();
    void update_jack_midi ();
    void okay ();
    void cancel ();
    void update_note_resume ();
    void update_key_height ();
    void update_ui_scaling (const QString &);
    void update_pattern_editor ();

    void on_spinBoxClockStartModulo_valueChanged (int arg1);
    void on_plainTextEditTempoTrack_textChanged ();
    void on_pushButtonTempoTrack_clicked ();
    void on_checkBoxRecordByChannel_clicked (bool checked);
    void on_chkJackConditional_stateChanged (int arg1);
};          // class qseditoptions

}           // namespace seq64

#endif      // SEQ64_QSEDITOPTIONS_HPP

/*
 * qseditoptions.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

