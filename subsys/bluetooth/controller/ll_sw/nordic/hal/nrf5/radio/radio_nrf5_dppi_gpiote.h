/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Extracted from radio_nrf_dppi.h functions that reference gpiote variables. */

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
static inline void hal_palna_ppi_setup(void)
{
	nrf_timer_publish_set(EVENT_TIMER, NRF_TIMER_EVENT_COMPARE2,
			      HAL_ENABLE_PALNA_PPI);
	nrf_radio_publish_set(NRF_RADIO, NRF_RADIO_EVENT_DISABLED,
			      HAL_DISABLE_PALNA_PPI);

	#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
		/* First hop: DPPIC10 -> DPPIC20 (required for all nRF54Lx FEM paths) */
		nrf_dppi_channels_enable(NRF_DPPIC20,
					BIT(HAL_ENABLE_PALNA_PPI) | BIT(HAL_DISABLE_PALNA_PPI));

		nrf_ppib_subscribe_set(NRF_PPIB11, HAL_PPIB_SEND_ENABLE_PALNA_PPI, HAL_ENABLE_PALNA_PPI);
		nrf_ppib_publish_set(NRF_PPIB21, HAL_PPIB_RECEIVE_ENABLE_PALNA_PPI, HAL_ENABLE_PALNA_PPI);

		nrf_ppib_subscribe_set(NRF_PPIB11, HAL_PPIB_SEND_DISABLE_PALNA_PPI, HAL_DISABLE_PALNA_PPI);
		nrf_ppib_publish_set(NRF_PPIB21, HAL_PPIB_RECEIVE_DISABLE_PALNA_PPI, HAL_DISABLE_PALNA_PPI);

		#if NRF_DT_GPIOTE_INST(FEM_NODE, HAL_RADIO_GPIO_PA_PROP) == 30
		/* Second hop: DPPIC20 -> DPPIC30 (FEM on gpio0/gpiote30, e.g. hwn001) */
		nrf_dppi_channels_enable(NRF_DPPIC30,
					BIT(HAL_ENABLE_PALNA_PPI) | BIT(HAL_DISABLE_PALNA_PPI));

		nrf_ppib_subscribe_set(NRF_PPIB22, HAL_PPIB_SEND_ENABLE_PALNA_PPI, HAL_ENABLE_PALNA_PPI);
		nrf_ppib_publish_set(NRF_PPIB30, HAL_PPIB_RECEIVE_ENABLE_PALNA_PPI, HAL_ENABLE_PALNA_PPI);

		nrf_ppib_subscribe_set(NRF_PPIB22, HAL_PPIB_SEND_DISABLE_PALNA_PPI, HAL_DISABLE_PALNA_PPI);
		nrf_ppib_publish_set(NRF_PPIB30, HAL_PPIB_RECEIVE_DISABLE_PALNA_PPI, HAL_DISABLE_PALNA_PPI);
		#endif /* NRF_DT_GPIOTE_INST == 30 */
	#endif /* CONFIG_SOC_COMPATIBLE_NRF54LX */

#if !defined(HAL_RADIO_FEM_IS_NRF21540)
	nrf_gpiote_task_t task;

	task = nrf_gpiote_out_task_get(gpiote_ch_palna);
	nrf_gpiote_subscribe_set(gpiote_palna.p_reg, task, HAL_DISABLE_PALNA_PPI);
#endif
}
#endif /* defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN) */

/******************************************************************************/
#if defined(HAL_RADIO_FEM_IS_NRF21540)
static inline void hal_gpiote_tasks_setup(NRF_GPIOTE_Type *gpiote,
					  uint8_t gpiote_ch,
					  bool inv,
					  uint8_t ppi_ch_enable,
					  uint8_t ppi_ch_disable)
{
	nrf_gpiote_task_t task;

	task = inv ? nrf_gpiote_clr_task_get(gpiote_ch) :
		     nrf_gpiote_set_task_get(gpiote_ch);
	nrf_gpiote_subscribe_set(gpiote, task, ppi_ch_enable);

	task = inv ? nrf_gpiote_set_task_get(gpiote_ch) :
		     nrf_gpiote_clr_task_get(gpiote_ch);
	nrf_gpiote_subscribe_set(gpiote, task, ppi_ch_disable);
}

static inline void hal_pa_ppi_setup(void)
{
	hal_gpiote_tasks_setup(gpiote_palna.p_reg, gpiote_ch_palna,
			       IS_ENABLED(HAL_RADIO_GPIO_PA_POL_INV),
			       HAL_ENABLE_PALNA_PPI, HAL_DISABLE_PALNA_PPI);
}

static inline void hal_lna_ppi_setup(void)
{
	hal_gpiote_tasks_setup(gpiote_palna.p_reg, gpiote_ch_palna,
			       IS_ENABLED(HAL_RADIO_GPIO_LNA_POL_INV),
			       HAL_ENABLE_PALNA_PPI, HAL_DISABLE_PALNA_PPI);
}

static inline void hal_fem_ppi_setup(void)
{
	nrf_timer_publish_set(EVENT_TIMER, NRF_TIMER_EVENT_COMPARE3,
			      HAL_ENABLE_FEM_PPI);
	nrf_radio_publish_set(NRF_RADIO, NRF_RADIO_EVENT_DISABLED,
			      HAL_DISABLE_FEM_PPI);

	#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
		/* Enable DPPI in Peripheral domain (20) */
		nrf_dppi_channels_enable(NRF_DPPIC20, BIT(HAL_ENABLE_FEM_PPI));

		/* Bridge SEND (Radio) -> RECEIVE (Peripheral) */
		nrf_ppib_subscribe_set(NRF_PPIB11, HAL_PPIB_SEND_ENABLE_FEM_PPI, HAL_ENABLE_FEM_PPI);
		nrf_ppib_publish_set(NRF_PPIB21, HAL_PPIB_RECEIVE_ENABLE_FEM_PPI, HAL_ENABLE_FEM_PPI);
	#endif

	hal_gpiote_tasks_setup(gpiote_pdn.p_reg, gpiote_ch_pdn,
			       IS_ENABLED(HAL_RADIO_GPIO_NRF21540_PDN_POL_INV),
			       HAL_ENABLE_FEM_PPI, HAL_DISABLE_FEM_PPI);
}

#endif /* HAL_RADIO_FEM_IS_NRF21540 */
