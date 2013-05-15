
static int tbs6992_set_frequency(struct dvb_frontend *fe, u32 frequency)
{
	struct dvb_frontend_ops	*frontend_ops = NULL;
	struct dvb_tuner_ops *tuner_ops = NULL;
	struct tuner_state t_state;
	int ret = 0;

	t_state.frequency = frequency;
	if (&fe->ops)
		frontend_ops = &fe->ops;
	if (&frontend_ops->tuner_ops)
		tuner_ops = &frontend_ops->tuner_ops;
	if (tuner_ops->set_state) {
		if ((ret = tuner_ops->set_state(fe, DVBFE_TUNER_FREQUENCY, &t_state)) < 0) {
			printk("%s: Invalid parameter\n", __func__);
			return ret;
		}
	}
	return 0;
}
