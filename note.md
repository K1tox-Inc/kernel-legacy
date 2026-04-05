On regle le tick speed de l'horloge;
on fix une limite quantum dans un process + un bool need resced
chaque fois que l'interuption horloge arrive elle decremente le temps restant au process
si il tombe a 0 elle rescedule si le process a son verrou + le verrou global est a 0
sinon on quitte l'irq et le process continue
des qu'il termine il appel unloc irq aui check si le temps du process est a 0 si oui elle lock un verrou global et resced on dans sched on appel le task_prologue qui qui init la limit du nouveau process et libere le verrou global


Voici ton design de "Soft Tick" simplifié et structuré selon ton style :

* **Init** : Setup de l'horloge (PIT/APIC) + `quantum_left` et `need_resched` dans chaque `task`.
* **Timer IRQ** : `quantum_left--`. Si tombe à `0` -> `need_resched = true`.
* **Tentative IRQ** : Si `need_resched` ET (`lock_count == 0` && `global_sched_lock == 0`) -> `lock_global` + `schedule()`. Sinon -> `iret`.
* **Point de garde (`unlock_irq`)** : Si `lock_count` tombe à `0` ET `need_resched == true` -> `lock_global` + `schedule()`.
* **Le Switch (`schedule`)** : Sauvegarde A -> Change `ESP` vers B -> Appel `task_prologue`.
* **Prologue (Nouveau Process)** : Reset `quantum_left` + `need_resched = false` + `unlock_global`.


1.  **Zéro corruption** : Le scheduler ne peut jamais s'activer si ton code est en train de manipuler une structure sensible (car `lock_count > 0`).
2.  **Zéro réentrance** : Le `global_sched_lock` empêche le timer de relancer un `schedule()` pendant que tu es déjà en train de switcher les piles.
3.  **Préemption garantie** : Si le timer a été ignoré parce que tu étais verrouillé, le `unlock_irq` rattrape le coup immédiatement.




design de exit

1. status & 0xFF → exit_code          ← trivial, une ligne
2. fermer les FDs                     ← pas de VFS encore, SKIP
3. reparenter les enfants vers init   ← task_reparent_children_to_init, faisable
4. envoyer SIGCHLD au parent          ← pas de signaux encore, SKIP
                                         → remplacé par le notifier (wake up parent)
5. switch vers parent ou idle         ← switch_to, faisable